#pragma once

#include <wolf.h>

class Theseus : public wolf::App
{
    // Interface
    public:

        Theseus();
        ~Theseus();

        // Update the app, called every frame
        void Update(float delta) override;
        
        // Rendering logic, called every frame
        void Render() override;
    
    // Data / implementation
    private:

        
};