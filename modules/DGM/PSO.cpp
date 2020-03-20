//
// Created by ahambasan on 22.02.20.
//
#include "PSO.h"
#include "macroses.h"

#include <random>   // TODO: you can use random namespace function from DGM/random.h

namespace DirectGraphicalModels 
{
    PSO::PSO(size_t nParams) 
        : CParamEstAlgorithm(nParams)
        , isThreadsEnabled(false)
    {
        reset();
    }

    // TODO: please substite this function with an implementatuion of setInitParams()
    PSO::PSO(const vec_float_t& vParams) 
        : CParamEstAlgorithm(vParams.size()) 
        , isThreadsEnabled(false)
    {
        // TODO: you can use random namespace function from DGM/random.h
        // SOURCE: https://en.cppreference.com/w/cpp/numeric/random/uniform_real_distribution
        std::random_device rd;  //Will be used to obtain a seed for the random number engine
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(-10.00f, 10.00f);

        // initialize boids
        for (auto i = 0; i < NUMBER_BOIDS; i++) {
            Boid* b_x = new Boid;
            for (auto d = 0; d < vParams.size(); d++) {
                b_x->pParams.push_back(dis(gen));
                b_x->velocity.push_back(1);
            }
            b_x->pBest = m_vParams;
            m_vBoids.push_back(*b_x);
        }

        gBest = m_vParams;

        reset();
    }

    void PSO::reset() {
        // initialize meta parameters
        c1 = C1_DEFAULT_VALUE;
        c2 = C2_DEFAULT_VALUE;
        w  = W_DEFAULT_VALUE;

        std::fill(m_vMin.begin(), m_vMin.end(), -FLT_MAX);
        std::fill(m_vMax.begin(), m_vMax.end(), FLT_MAX);
        std::fill(m_vParams.begin(), m_vParams.end(), 0.0f);
    }

    vec_float_t PSO::getParams(std::function<float(vec_float_t)> objectiveFunct) {
        if (isThreadsEnabled) {
            std::vector<std::thread> threads_v;
            for (size_t i = 0; i < NUMBER_BOIDS; i++) {
                threads_v.emplace_back(&DirectGraphicalModels::PSO::runPSO_withThreads,
                    this, objectiveFunct, i);
            }
            std::for_each(threads_v.begin(), threads_v.end(), [](std::thread& th) {
                th.join();
                });

            return gBest;
        }
        else {
            runPSO(objectiveFunct);
            return gBest;
        }
    }

    void PSO::runPSO(const std::function<float(vec_float_t)>& objectiveFunct) {
        size_t it = 0;
        while (it < MAX_NR_ITERATIONS) {
            for (auto& boid : m_vBoids) {
                float objectiveFunct_val = objectiveFunct(boid.pParams);
                float pBest_val = objectiveFunct(boid.pBest);
                if (objectiveFunct_val < pBest_val) {
                    boid.pBest = boid.pParams;
                }

                float gBest_val = objectiveFunct(gBest);
                if (objectiveFunct_val < gBest_val) {
                    gBest = boid.pBest;
                }
            }
            for (auto& boid : m_vBoids) {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_real_distribution<> dis(0, 1);

                float r1 = dis(gen);
                float r2 = dis(gen);
                for (auto d = 0; d < m_vParams.size(); d++) {
                    boid.velocity[d] = w * boid.velocity[d] + c1 * r1 * (boid.pBest[d] - boid.pParams[d]) +
                        c2 * r2 * (gBest[d] - boid.pParams[d]);
                    boid.pParams[d] = boid.pParams[d] + boid.velocity[d];
                }
            }

            it++;
        }
    }

    void PSO::runPSO_withThreads(const std::function<float(vec_float_t)>& objectiveFunct,
        size_t idx) {
        size_t it = 0;
        while (it < MAX_NR_ITERATIONS) {
            float objectiveFunct_val = objectiveFunct(m_vBoids[idx].pParams);
            float pBest_val = objectiveFunct(m_vBoids[idx].pBest);
            if (objectiveFunct_val < pBest_val) {
                m_vBoids[idx].pBest = m_vBoids[idx].pParams;
            }

            mtx.lock();
            float gBest_val = objectiveFunct(gBest);
            if (objectiveFunct_val < gBest_val) {
                gBest = m_vBoids[idx].pBest;
            }
            mtx.unlock();


            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<> dis(0, 1);

            float r1 = dis(gen);
            float r2 = dis(gen);
            for (auto d = 0; d < m_vParams.size(); d++) {
                m_vBoids[idx].velocity[d] = w * m_vBoids[idx].velocity[d] + c1 * r1 * (m_vBoids[idx].pBest[d] - m_vBoids[idx].pParams[d]) +
                    c2 * r2 * (gBest[d] - m_vBoids[idx].pParams[d]);
                m_vBoids[idx].pParams[d] = m_vBoids[idx].pParams[d] + m_vBoids[idx].velocity[d];
            }

            it++;
        }
    }

    bool PSO::isMultiThreadingEnabled() const {
        return this->isThreadsEnabled;
    }

    void PSO::enableMultiThreading() {
        this->isThreadsEnabled = true;
    }

    void PSO::enableMultiThreading(bool enable) {
        this->isThreadsEnabled = enable;
    }

    void PSO::disableMultiThreading() {
        this->isThreadsEnabled = false;
    }

}