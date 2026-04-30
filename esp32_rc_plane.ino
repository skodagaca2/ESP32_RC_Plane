// esp32_rc_plane.ino

void setup() {
    // Your setup code here
}

void loop() {
    int currentLeft = 0; // Example value
    int targetLeft = 10; // Example target
    int stepPerInterval = 2; // Example step

    // Change all std::min and std::max calls
    int leftControl = std::min(currentLeft + stepPerInterval, (int)targetLeft);
    // int rightControl = std::max(currentRight - stepPerInterval, (int)targetRight);
}
