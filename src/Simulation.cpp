// Simulation.cpp
#include "../include/Simulation.h"
#include "../include/ParticleSystem.h"
#include "../include/Globals.h"
#include <cmath>
#include <iostream>
#include <fstream>


float SPHComputations::kernelAlpha() {
    return 5 / (14 * M_PI * std::pow(h, 2));
}

float SPHComputations::kernel(sf::Vector2f positionA,sf::Vector2f positionB) {
    float distance = euclideanDistance(positionA, positionB);
    float q = distance / h;

    float result = 0.0f;
    if (q < 0.0f) {
        throw std::runtime_error("Invalid distance");
    } else if (q < 1.0f) {
        result = kernelAlpha() * (std::pow((2 - q),3) - 4 * std::pow((1 - q), 3));
    } else if (q < 2.0f) {
        result = kernelAlpha() * std::pow((2 - q),3);
    } else {
        result = 0;
    }
    //printf("Kernel Value for distance %f: %f and q = %f \n", distance, result, q);
    return result;
}

sf::Vector2f SPHComputations::kernelGradient(sf::Vector2f positionA,sf::Vector2f positionB) {
    float distance = euclideanDistance(positionA, positionB);
    float q = distance / h;
    sf::Vector2f diff = positionA - positionB;

    // if difference == 0 return (0,0)
    if (diff == sf::Vector2f(0.0f, 0.0f)) {
        return sf::Vector2f(0.0f, 0.0f);
    }

    if (q < 0.0f) {
        throw std::runtime_error("Invalid distance");
    } else if (q < 1.0f) {
        return (kernelAlpha() * (diff / (distance * h))) * (-3 * std::pow((2 - q),2) + 12 * std::pow((1- q), 2));
    }
        else if (q < 2.0f) {
        return (kernelAlpha() * (diff / (distance * h))) * (-3 * std::pow((2 - q),2));
    } else {
        return sf::Vector2f(0.0f, 0.0f);
    }
}

// old version
// float SPHComputations::computeDensity(Particle& particle, std::vector<Particle>& neighbours) {
//     float density_i = 0.0f;
//     float density_fluid = 0.0f;
//     float density_boundary = 0.0f;
//     for (auto& particle_j : neighbours) {
//         if (particle_j.isStatic == false) {
//             float kernel_value = SPHComputations::kernel(particle.position, particle_j.position);
//             float contribution = particle_j.mass * kernel_value;
//             density_fluid += contribution;
//         }
//         if (particle_j.isStatic == true) {
//             float kernel_value = SPHComputations::kernel(particle.position, particle_j.position);
//             float contribution = particle_j.mass * kernel_value;
//             density_boundary += contribution;
//         }

//     }
//     return density_fluid + density_boundary;
// }

// new version gamma
// float SPHComputations::computeDensity(Particle& particle, std::vector<Particle>& neighbours) {
//     float density_i = 0.0f;
//     for (auto& particle_j : neighbours) {
//         if (particle_j.isStatic == false) {
//             float kernel_value = SPHComputations::kernel(particle.position, particle_j.position);
//             float contribution = particle_j.mass * kernel_value;
//             density_i += contribution;
//             //printf("Neighbour Position: (%f, %f), Kernel Value: %f, Contribution to Density: %f\n",
//             //       particle_j.position.x, particle_j.position.y, kernel_value, contribution);
//         }
//         if (particle_j.isStatic == true){
//             float kernel_value = SPHComputations::kernel(particle.position, particle_j.position);
//             float contribution = gamma_1 * particle_j.mass * kernel_value;
//             density_i += contribution;
//         }
//     }
//     return density_i;
// }

// fixed version
float SPHComputations::computeDensity(Particle& particle, std::vector<Particle>& neighbours) {
    float density_i = 0.0f;
    for (auto& particle_j : neighbours) {
        float kernel_value = SPHComputations::kernel(particle.position, particle_j.position);
        float contribution = particle_j.mass * kernel_value;
        density_i += contribution;
    }
    return density_i;
}


bool SPHComputations::isParticleCompressed(float density_i) {
    return density_i >= density_rest;
}

float SPHComputations::computePressure(float density_i) {
    float pressure_i;
    pressure_i = stiffness_constant_k *(std::pow((density_i/density_rest),7) -1);
    // pressure_i = stiffness_constant_k *((density_i/density_rest) -1);
    if (pressure_i < 0) {
        return 0;
    }
    return pressure_i;
}

// sf::Vector2f SPHComputations::computePressureAcceleration(Particle& particle_i, std::vector<Particle>& neighbours, float pressure_i) {
//     sf::Vector2f pressureAcceleration(0.0f, 0.0f);
//     sf::Vector2f sum(0.0f, 0.0f);

//     for (auto& particle_j : neighbours) {
//         if (particle_j.isStatic == true) {
//             sum += kernelGradient(particle_i.position, particle_j.position);
//             pressureAcceleration += (pressure_i * ((2 * particle_i.mass) / std::pow(particle_i.density, 2))) * sum;
//         }
//         pressureAcceleration += particle_j.mass * ((particle_i.pressure / std::pow(particle_i.density, 2)) 
//                                                     +
//                                                      (particle_j.pressure / std::pow(particle_j.density, 2)))
//                                                      * kernelGradient(particle_i.position, particle_j.position);

//     }

//     return -1 * pressureAcceleration;
// }

// old version
// sf::Vector2f SPHComputations::computePressureAcceleration(Particle& particle_i, std::vector<Particle>& neighbours, float pressure_i) {
//     sf::Vector2f pressureAccelerationFluid(0.0f, 0.0f);
//     sf::Vector2f pressureAccelerationBoundary(0.0f, 0.0f);

//     for (auto& particle_j : neighbours) {
//         if (particle_j.isStatic == false) {
//             pressureAccelerationFluid += particle_j.mass * ((particle_i.pressure / std::pow(particle_i.density, 2)) 
//                                                         +
//                                                         (particle_j.pressure / std::pow(particle_j.density, 2)))
//                                                         * kernelGradient(particle_i.position, particle_j.position);
//         }
//         if (particle_j.isStatic == true) {
//             pressureAccelerationBoundary += particle_j.mass * ((particle_i.pressure / std::pow(particle_i.density, 2)) 
//                                                         +
//                                                         (particle_j.pressure / std::pow(particle_j.density, 2)))
//                                                         * kernelGradient(particle_i.position, particle_j.position);
//         }

//     }

//     return -1 * pressureAccelerationFluid + -1 * pressureAccelerationBoundary;

// }

// new Version with boundary handling gamma
// sf::Vector2f SPHComputations::computePressureAcceleration(Particle& particle_i, std::vector<Particle>& neighbours, float pressure_i) {
//     sf::Vector2f pressureAccelerationFluid(0.0f, 0.0f);
//     sf::Vector2f pressureAccelerationBoundary(0.0f, 0.0f);
//     sf::Vector2f sumBoundary(0.0f, 0.0f);
//     sf::Vector2f sumFluid(0.0f, 0.0f);

//     for (auto& particle_j : neighbours) {

//         if (particle_j.isStatic == true) {
//             sumBoundary += (particle_j.mass * kernelGradient(particle_i.position, particle_j.position));
//         }
//         if (particle_j.isStatic == false) {
//             sumFluid += ((particle_i.pressure / std::pow(particle_i.density, 2)) +
//                         (particle_j.pressure / std::pow(particle_j.density, 2)))
//                         * kernelGradient(particle_i.position, particle_j.position);
//         }
//     }
//     pressureAccelerationBoundary = (pressure_i * ((2 * gamma_2) / std::pow(particle_i.density, 2))) * sumBoundary;
//     pressureAccelerationFluid    = particle_i.mass * sumFluid;
    

//     return (-pressureAccelerationFluid) - pressureAccelerationBoundary;

// }

// fixed verison
sf::Vector2f SPHComputations::computePressureAcceleration(Particle& particle_i, std::vector<Particle>& neighbours, float pressure_i) {
    sf::Vector2f pressureAccelerationFluid(0.0f, 0.0f);
    sf::Vector2f pressureAccelerationBoundary(0.0f, 0.0f);

    for (auto& particle_j : neighbours) {
        if (particle_j.isStatic == false) {
            pressureAccelerationFluid += particle_i.mass * ((particle_i.pressure / std::pow(particle_i.density, 2)) 
                                                        +
                                                        (particle_j.pressure / std::pow(particle_j.density, 2)))
                                                        * kernelGradient(particle_i.position, particle_j.position);

        }
        if (particle_j.isStatic == true) {
            pressureAccelerationFluid += particle_i.pressure * ((2 * particle_i.mass)/(std::pow(particle_i.density,2))) 
                                        * kernelGradient(particle_i.position, particle_j.position);
        }
    }

    return - pressureAccelerationFluid - pressureAccelerationBoundary;
}


// old version viscosity also at boundary
// sf::Vector2f SPHComputations::computeViscosity(Particle& particle_i, std::vector<Particle>& neighbours) {
//     sf::Vector2f sum(0.0f, 0.0f);
//     for (auto& particle_j : neighbours) {
//         sf::Vector2f v_diff = particle_i.velocity - particle_j.velocity;
//         sf::Vector2f x_diff = particle_i.position - particle_j.position;

//         sum += (particle_j.mass / particle_j.density) 
//                 * ((v_diff * x_diff)/(x_diff * x_diff + 0.01f * std::pow((h),2)))
//                 * kernelGradient(particle_i.position, particle_j.position);
//     }

//     printf("sum: %f %f\n", sum.x, sum.y);

//     return 2 * viscosityFactor * sum;
// }

// new version as test boundary no visco
// sf::Vector2f SPHComputations::computeViscosity(Particle& particle_i, std::vector<Particle>& neighbours) {
//     sf::Vector2f sum(0.0f, 0.0f);

//     for (auto& particle_j : neighbours) {
//         if (particle_j.isStatic == false) {
//             sf::Vector2f v_diff = particle_i.velocity - particle_j.velocity;
//             sf::Vector2f x_diff = particle_i.position - particle_j.position;

//             sum += (particle_j.mass / particle_j.density) 
//                     * ((v_diff * x_diff)/(x_diff * x_diff + std::pow((0.01*h),2)))
//                     * kernelGradient(particle_i.position, particle_j.position);
//             }   
//     }

//     return 2 * viscosityFactor * sum;
// }

// fixed version
sf::Vector2f SPHComputations::computeViscosity(Particle& particle_i, std::vector<Particle>& neighbours) {
    sf::Vector2f sumFluid(0.0f, 0.0f);
    sf::Vector2f sumBoundary(0.0f, 0.0f);
    for (auto& particle_j : neighbours) {
        if (particle_j.isStatic == false) {
            sf::Vector2f v_diff = particle_i.velocity - particle_j.velocity;
            sf::Vector2f x_diff = particle_i.position - particle_j.position;

            sumFluid += (particle_j.mass / particle_j.density) 
                    * ((v_diff * x_diff)/(x_diff * x_diff + 0.01f * std::pow((h),2)))
                    * kernelGradient(particle_i.position, particle_j.position);
        }
        if (particle_j.isStatic == true) {
            sf::Vector2f v_diff = particle_i.velocity - particle_j.velocity;
            sf::Vector2f x_diff = particle_i.position - particle_j.position;

            sumBoundary += (particle_j.mass / particle_j.density) 
                    * ((v_diff * x_diff)/(x_diff * x_diff + 0.01f * std::pow((h),2)))
                    * kernelGradient(particle_i.position, particle_j.position);
        }
    }

    return (2 * viscosityFactor * sumFluid) + (viscosityFactor * sumBoundary);
}




sf::Vector2f SPHComputations::computeSurfaceTension(Particle& particle_i, std::vector<Particle>& neighbours) {
    sf::Vector2f sum(0.0f, 0.0f);

    for (auto& particle_j : neighbours) {
        if (particle_j.isStatic == false) {
            sum += kernelGradient(particle_i.position, particle_j.position); 
        }
    }
    if (sum != sf::Vector2f(0.0f, 0.0f)) {
        return sum;
    } else {
        return sf::Vector2f(0.0f, 0.0f);
    }
}

sf::Vector2f SPHComputations::computeTotalAcceleration(Particle& particle, std::vector<Particle>& neighbours) {
    sf::Vector2f pressureAcceleration = computePressureAcceleration(particle, neighbours, particle.pressure);
    sf::Vector2f viscosityAcceleration = computeViscosity(particle, neighbours);

    sf::Vector2f surfaceTension = computeSurfaceTension(particle, neighbours);

    sf::Vector2f totalAcceleration = pressureAcceleration + viscosityAcceleration + gravity + surfaceTension * surfaceTensionFactor;

    return totalAcceleration;
}

void SPHComputations::advectParticles(std::vector<Particle>& particles) {
    for (auto& particle : particles) {
        if (particle.isStatic) {
            continue;
        }
        particle.velocity = particle.velocity + timeStep * particle.acceleration;
        particle.position = particle.position + timeStep * particle.velocity;
    }
}

// for color gradient
float speedThreshold1 = 20.0f;  // Blue to Cyan
float speedThreshold2 = 40.0f;  // Cyan to Green

// max velo and color gradient to speed
float SPHComputations::getMaxVelocity(std::vector<Particle>& particles) {
    float maxSpeed = 0;
    float speed = 0;
    float CFLNumber = 0;

    for (auto& particle : particles) {
        if (particle.isStatic) {
            continue;
        }

        speed = std::sqrt(particle.velocity.x * particle.velocity.x + particle.velocity.y * particle.velocity.y);
        if (speed > maxSpeed) {
            maxSpeed = speed;
        }
    }

    // Loop through each particle to apply the color based on the absolute speed
    for (auto& particle : particles) {
        if (particle.isStatic) continue;

        // Calculate the speed of the particle
        float speed = std::sqrt(particle.velocity.x * particle.velocity.x + particle.velocity.y * particle.velocity.y);

        sf::Uint8 r, g, b;

        if (speed < speedThreshold1) {
            // Blue to Cyan
            float ratio = speed / speedThreshold1; // [0, speedThreshold1] -> [0, 1]
            r = 0;
            g = static_cast<sf::Uint8>(ratio * 255);
            b = 255;
        } else if (speed < speedThreshold2) {
            // Cyan to Green
            float ratio = (speed - speedThreshold1) / (speedThreshold2 - speedThreshold1); // [speedThreshold1, speedThreshold2] -> [0, 1]
            r = 0;
            g = 255;
            b = static_cast<sf::Uint8>((1 - ratio) * 255);
        } else {
            // Green to Red
            float ratio = (speed - speedThreshold2) / (maxSpeed - speedThreshold2); // [speedThreshold2, maxSpeed] -> [0, 1]
            r = static_cast<sf::Uint8>(ratio * 255);
            g = static_cast<sf::Uint8>((1 - ratio) * 255);
            b = 0;
        }

        // Set the particle's color
        particle.color = sf::Color(r, g, b, 255);
    }


    // color according to cfl number
    // for (auto& particle : particles) {
    //     if (particle.isStatic) continue;
    //     speed = std::sqrt(particle.velocity.x * particle.velocity.x + particle.velocity.y * particle.velocity.y);
    //     float normalizedSpeed = speed / maxSpeed;
    //     if (normalizedSpeed < 0.5f) {
    //     // Blue to White
    //     float ratio = normalizedSpeed * 2.0f; // [0, 0.5] -> [0, 1]
    //     sf::Uint8 r = static_cast<sf::Uint8>(ratio * 255);
    //     sf::Uint8 g = static_cast<sf::Uint8>(ratio * 255);
    //     particle.color = sf::Color(r, g, 255, 255);
    //     } else {
    //         // White to Red
    //         float ratio = (normalizedSpeed - 0.5f) * 2.0f; // [0.5, 1] -> [0, 1]
    //         sf::Uint8 r = 255;
    //         sf::Uint8 g = static_cast<sf::Uint8>((1 - ratio) * 255);
    //         particle.color = sf::Color(r, g, g, 255);
    //     }
    // }

    // for (auto& particle : particles) {
    //     if (particle.isStatic) continue;
    //     speed = std::sqrt(particle.velocity.x * particle.velocity.x + particle.velocity.y * particle.velocity.y);
    //     CFLNumber = timeStep * (h / speed);
    //     float normalizedSpeed = (speed-0)/(100-0); //min-max normalization (0-100)
    //     //printf("speed: %f", normalizedSpeed);
    //     if (normalizedSpeed < 10.0f) {
    //         float ratio = normalizedSpeed * 2.0f; // [0, 0.5] -> [0, 1]
    //         sf::Uint8 r = static_cast<sf::Uint8>(ratio * 255);
    //         sf::Uint8 g = static_cast<sf::Uint8>(ratio * 255);
    //         particle.color = sf::Color(r, g, 255, 255);
    //     }
    //     else if (normalizedSpeed >= 10.0f && speed < 60.0f) {
    //         float ratio = normalizedSpeed * 2.0f; // [0, 0.5] -> [0, 1]
    //         sf::Uint8 r = static_cast<sf::Uint8>(ratio * 255);
    //         sf::Uint8 g = static_cast<sf::Uint8>(ratio * 255);
    //         particle.color = sf::Color(r, 255, g, 255);
    //     }
    //     else {
    //         float ratio = (normalizedSpeed - 0.5f) * 2.0f; // [0.5, 1] -> [0, 1]
    //         sf::Uint8 r = 255;
    //         sf::Uint8 g = static_cast<sf::Uint8>((1 - ratio) * 255);
    //         particle.color = sf::Color(r, g, g, 255);
    //     }
    // }
    
    // for (auto& particle : particles) {
    //     if (particle.isStatic) continue;
    //     speed = std::sqrt(particle.velocity.x * particle.velocity.x + particle.velocity.y * particle.velocity.y);
    //     float normalizedSpeed = (speed-0)/(100-0); //min-max normalization min:0 max:100 selbstgesetzte werte
    //     if (normalizedSpeed <= 0.02f) {
    //         float ratio = normalizedSpeed / 0.05f;
    //         sf::Uint8 r = static_cast<sf::Uint8>(0 * (1 - ratio) + 0 * ratio);
    //         sf::Uint8 g = static_cast<sf::Uint8>(0 * (1 - ratio) + 255 * ratio);
    //         sf::Uint8 b = static_cast<sf::Uint8>(255 * (1 - ratio) + 255 * ratio);
    //         particle.color = sf::Color(r, g, b, 255);
    //     }
    //     else if (normalizedSpeed <= 0.2f) {
    //         float ratio = normalizedSpeed - 0.1f / 0.05f;
    //         sf::Uint8 r = static_cast<sf::Uint8>(0 * (1 - ratio) + 0 * ratio);
    //         sf::Uint8 g = static_cast<sf::Uint8>(255 * (1 - ratio) + 255 * ratio);
    //         sf::Uint8 b = static_cast<sf::Uint8>(255 * (1 - ratio) + 0 * ratio);
    //         particle.color = sf::Color(r, g, b, 255);
    //     }
    //     else if (normalizedSpeed <= 0.3f) {
    //         float ratio = normalizedSpeed - 0.3f / 0.05f;
    //         sf::Uint8 r = static_cast<sf::Uint8>(0 * (1 - ratio) + 255 * ratio);
    //         sf::Uint8 g = static_cast<sf::Uint8>(255 * (1 - ratio) + 255 * ratio);
    //         sf::Uint8 b = static_cast<sf::Uint8>(0 * (1 - ratio) + 0 * ratio);
    //         particle.color = sf::Color(r, g, b, 255);
    //     }
    //     else if (normalizedSpeed <= 0.4f) {
    //         float ratio = normalizedSpeed - 0.5f / 0.05f;
    //         sf::Uint8 r = static_cast<sf::Uint8>(255 * (1 - ratio) + 255 * ratio);
    //         sf::Uint8 g = static_cast<sf::Uint8>(255 * (1 - ratio) + 165 * ratio);
    //         sf::Uint8 b = static_cast<sf::Uint8>(0 * (1 - ratio) + 0 * ratio);
    //         particle.color = sf::Color(r, g, b, 255);
    //     }
    //     else {
    //         float ratio = normalizedSpeed - 0.7f / 0.05f;
    //         sf::Uint8 r = static_cast<sf::Uint8>(255 * (1 - ratio) + 255 * ratio);
    //         sf::Uint8 g = static_cast<sf::Uint8>(165 * (1 - ratio) + 0 * ratio);
    //         sf::Uint8 b = static_cast<sf::Uint8>(0 * (1 - ratio) + 0 * ratio);
    //         particle.color = sf::Color(r, g, b, 255);
    //     }
    // }

    return maxSpeed;
}

void SPHComputations::isCFLConditionTrue(std::vector<Particle>& particles) {
    float lambda = 0.9f;
    float value;
    // value = lambda * (h / getMaxVelocity(particles));
    // printf("timeStep: %f  CFL: %f    maxVeloc: %f \n", timeStep, value, getMaxVelocity(particles));
    if (timeStep <= lambda * (h / getMaxVelocity(particles))) {
        CFLCondition = true;
    } else {
        CFLCondition = false;
    }
}

float SPHComputations::getCourantNumber(std::vector<Particle>& particles) {
    float value;
    value = timeStep * (h / getMaxVelocity(particles));
    return value;
}

float SPHComputations::getAvgDensity(std::vector<Particle>& particles, const std::string& filename) {
    float avgDensity = 0.0f;
    int count = 0;
    for (auto& particle : particles) {
        if (particle.isStatic) continue;
        if (particle.density < density_rest) continue;
        // if (particle.density > density_rest) {
        //     avgDensity += particle.density;
        //     count++;
        // }
        avgDensity += particle.density;
        count++;

    }

    avgDensity /= count;

    // Append the average density value to the text file
    std::ofstream outFile(filename, std::ios_base::app);
    if (outFile.is_open()) {
        outFile << avgDensity << "\n";
        outFile.close();
    } else {
        std::cerr << "Unable to open file " << filename << std::endl;
    }

    return avgDensity;
}

// float SPHComputations::getRelativeDensityError(std::vector<Particle>& particles, const std::string& filename) {
//     float relDensity = 0.0f;
//     int count = 0;
//     for (auto& particle : particles) {
//         if (particle.isStatic) continue;
//         if (particle.density < density_rest) continue;

//         relDensity = 

//     }

//     relDensity /= particles.size();

//     // Append the average density value to the text file
//     std::ofstream outFile(filename, std::ios_base::app);
//     if (outFile.is_open()) {
//         outFile << relDensity << "\n";
//         outFile.close();
//     } else {
//         std::cerr << "Unable to open file " << filename << std::endl;
//     }

//     return relDensity;
// }