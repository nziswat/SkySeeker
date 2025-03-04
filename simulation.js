// simulation.js

// Doesnt rotate and heading doesnt update correctly

// Sleep function 
function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

// Function to make the plane move in a circle
async function circle(plane, a, b, r, n, loops, s, movingMarker) {
    for (let i = 0; i < n * loops; i++) {

        // Calculate the angle for the current step
        const angle = (2 * Math.PI * i) / n;

        // Update plane's coordinates
        plane.long = a + r * Math.cos(angle);
        plane.lat = b + r * Math.sin(angle);

        // Update plane's heading based on the current angle
        const heading = (angle * 180 / Math.PI) % 360;  // Convert the angle directly to degrees
        plane.head = heading;

        // Move the plane to new position
        movingMarker.moveTo([plane.lat, plane.long], {
            duration: s/2 * 1000, // Duration of the move in milliseconds (Note: s/2 to show each individual spot)
        });

        // Apply rotation (adjust heading by -45 degrees to match the up-right facing icon)
        //rotateMarker(movingMarker, plane.head);

        // Display current info
        console.log("Plane position:", plane.lat, plane.long, "Heading:", plane.head);

        // Wait for the specified time before the next update
        await sleep(s * 1000);
    }
}

// Function to initialize the map
function initializeMap(containerId, center, zoom) {
    // Initialize the map
    const map = L.map(containerId).setView(center, zoom);

    // Add a tile layer
    const tiles = L.tileLayer('https://tile.openstreetmap.org/{z}/{x}/{y}.png', {
        maxZoom: 19,
        attribution: '&copy; <a href="http://www.openstreetmap.org/copyright">OpenStreetMap</a>'
    }).addTo(map);

    return map;
}
/*
// Function to rotate position of the aircraft icon
function rotateMarker(movingMarker, angle) {
    const markerElement = movingMarker._icon;
    if (markerElement) {
        markerElement.style.transform = `rotate(${angle - 45}deg)`; // Adjust for up-right facing icon
        markerElement.style.transformOrigin = "50% 50%"; // Ensure rotation happens at the center
    }
}
*/