// physics_test.cpp
// Test harness for verifying lunar lander physics accuracy

#include <iostream>
#include <iomanip>
#include <cmath>

// Lunar gravity constant (m/s²)
const float LUNAR_GRAVITY = 1.62f;

// Test free fall calculation
void test_free_fall(float height, float gravity) {
    // Calculate free fall time: t = sqrt(2h/g)
    float time = sqrt(2.0f * height / gravity);
    
    std::cout << "===== FREE FALL TEST =====" << std::endl;
    std::cout << "Initial height: " << height << " m" << std::endl;
    std::cout << "Gravity: " << gravity << " m/s²" << std::endl;
    std::cout << "Theoretical time to impact: " << time << " s" << std::endl;
    
    // Simulate the fall using small time steps
    float h = height;
    float v = 0.0f;
    float t = 0.0f;
    float dt = 0.01f; // 10ms timestep
    
    std::cout << "\nSimulated fall:" << std::endl;
    std::cout << "Time (s) | Height (m) | Velocity (m/s)" << std::endl;
    std::cout << "--------------------------------------" << std::endl;
    
    while (h > 0) {
        // Print state every 0.5 seconds
        if (fabs(t - round(t * 2) / 2) < dt/2) {
            std::cout << std::fixed << std::setprecision(2)
                      << std::setw(8) << t << " | "
                      << std::setw(10) << h << " | "
                      << std::setw(14) << v << std::endl;
        }
        
        // Update velocity: v = v + g*dt
        v += gravity * dt;
        
        // Update height: h = h - v*dt
        h -= v * dt;
        
        // Update time
        t += dt;
    }
    
    std::cout << std::fixed << std::setprecision(2)
              << std::setw(8) << t << " | "
              << std::setw(10) << 0.00 << " | "
              << std::setw(14) << v << std::endl;
              
    std::cout << "\nSimulated impact time: " << t << " s" << std::endl;
    std::cout << "Impact velocity: " << v << " m/s" << std::endl;
    std::cout << "Theoretical impact velocity: " << (gravity * time) << " m/s" << std::endl;
    
    // Calculate error percentage
    float timeError = 100.0f * fabs(t - time) / time;
    std::cout << "Time error: " << timeError << "%" << std::endl;
}

// Test projectile motion
void test_projectile(float initialVelocity, float angle, float gravity) {
    // Convert angle to radians
    float angleRad = angle * M_PI / 180.0f;
    
    // Initial velocity components
    float vx = initialVelocity * cos(angleRad);
    float vy = initialVelocity * sin(angleRad);
    
    // Calculate theoretical range: R = v²*sin(2θ)/g
    float range = (initialVelocity * initialVelocity * sin(2 * angleRad)) / gravity;
    
    // Calculate theoretical max height: h = v²*sin²(θ)/(2g)
    float maxHeight = (initialVelocity * initialVelocity * sin(angleRad) * sin(angleRad)) / (2 * gravity);
    
    // Calculate time of flight: t = 2v*sin(θ)/g
    float timeOfFlight = (2 * initialVelocity * sin(angleRad)) / gravity;
    
    std::cout << "===== PROJECTILE MOTION TEST =====" << std::endl;
    std::cout << "Initial velocity: " << initialVelocity << " m/s at " << angle << " degrees" << std::endl;
    std::cout << "Gravity: " << gravity << " m/s²" << std::endl;
    std::cout << "Theoretical range: " << range << " m" << std::endl;
    std::cout << "Theoretical max height: " << maxHeight << " m" << std::endl;
    std::cout << "Theoretical time of flight: " << timeOfFlight << " s" << std::endl;
    
    // Simulate the projectile using small time steps
    float x = 0.0f;
    float y = 0.0f;
    float t = 0.0f;
    float dt = 0.01f; // 10ms timestep
    
    std::cout << "\nSimulated trajectory:" << std::endl;
    std::cout << "Time (s) | X (m) | Y (m) | Vx (m/s) | Vy (m/s)" << std::endl;
    std::cout << "------------------------------------------------" << std::endl;
    
    float maxY = 0.0f;
    
    while (y >= 0 || t == 0) {
        // Print state every 0.5 seconds
        if (fabs(t - round(t * 2) / 2) < dt/2) {
            std::cout << std::fixed << std::setprecision(2)
                      << std::setw(8) << t << " | "
                      << std::setw(5) << x << " | "
                      << std::setw(5) << y << " | "
                      << std::setw(8) << vx << " | "
                      << std::setw(8) << vy << std::endl;
        }
        
        // Update velocity: v = v + g*dt (only y-component)
        vy -= gravity * dt;
        
        // Update position: p = p + v*dt
        x += vx * dt;
        y += vy * dt;
        
        // Track maximum height
        if (y > maxY) maxY = y;
        
        // Update time
        t += dt;
        
        // Prevent infinite loop if numerical errors keep y just above 0
        if (t > timeOfFlight * 2) break;
    }
    
    // Interpolate to find exact landing position and time
    float tLand = t - dt - (y + vy * dt) / vy;
    float xLand = x - vx * dt + vx * (tLand - (t - dt));
    
    std::cout << std::fixed << std::setprecision(2)
              << std::setw(8) << tLand << " | "
              << std::setw(5) << xLand << " | "
              << std::setw(5) << 0.00 << " | "
              << std::setw(8) << vx << " | "
              << std::setw(8) << (vy - gravity * (tLand - (t - dt))) << std::endl;
              
    std::cout << "\nSimulated range: " << xLand << " m" << std::endl;
    std::cout << "Simulated max height: " << maxY << " m" << std::endl;
    std::cout << "Simulated time of flight: " << tLand << " s" << std::endl;
    
    // Calculate error percentages
    float rangeError = 100.0f * fabs(xLand - range) / range;
    float heightError = 100.0f * fabs(maxY - maxHeight) / maxHeight;
    float timeError = 100.0f * fabs(tLand - timeOfFlight) / timeOfFlight;
    
    std::cout << "Range error: " << rangeError << "%" << std::endl;
    std::cout << "Height error: " << heightError << "%" << std::endl;
    std::cout << "Time error: " << timeError << "%" << std::endl;
}

int main() {
    std::cout << "Lunar Lander Physics Test Harness" << std::endl;
    std::cout << "--------------------------------" << std::endl;
    
    // Test free fall with lunar gravity
    test_free_fall(5.0f, LUNAR_GRAVITY); // 5m drop on the Moon
    
    std::cout << "\n\n";
    
    // Test projectile motion with lunar gravity
    test_projectile(10.0f, 45.0f, LUNAR_GRAVITY); // 10 m/s at 45 degrees
    
    return 0;
}
