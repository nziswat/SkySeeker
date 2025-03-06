// Sleep function 
function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

// Function to update the table
function updateTable(plane) {
    document.getElementById('planeID').textContent = plane.ID;
    document.getElementById('planeLat').textContent = plane.lat.toFixed(6);
    document.getElementById('planeLong').textContent = plane.long.toFixed(6);
    document.getElementById('planeHead').textContent = plane.head.toFixed(2);
    document.getElementById('planeAlt').textContent = plane.alt;
    document.getElementById('planeSpeed').textContent = plane.speed;
}

// Function to make the plane move in a circle
async function circle(plane, a, b, r, n, loops, s, movingMarker, polyline, map) {
    for (let i = 0; i < n * loops; i++) {

        // Calculate the angle for the current step
        const angle = (2 * Math.PI * i) / n;

        // Update plane's coordinates
        plane.long = a + r * Math.cos(angle);
        plane.lat = b + r * Math.sin(angle);

        // Update plane's heading based on the current angle
        const heading = (360 - (angle * 180 / Math.PI) % 360) % 360;  // Convert the angle directly to degrees
        plane.head = heading;

        // Move the plane to new position
        movingMarker.setLatLng([plane.lat, plane.long]);

        // Rotate the plane using the RotatedMarker plugin (instead of manually updating CSS)
        movingMarker.setRotationAngle(plane.head - 45); // Rotate the plane icon based on heading

        // Update polyline
        polyline.addLatLng([plane.lat, plane.long]);

        // Update the table with the current information
        updateTable(plane);

        // Wait for the specified time before the next update
        await sleep(s * 1000);
    }
}

// Function to initialize the map
function initializeMap(containerId, center, zoom) {

    var options = {};
    options.worldCopyJump = true; // Makes the markers repeat

    // Initialize the map
    const map = L.map(containerId, options).setView(center, zoom);

    // Add a tile layer
    const tiles = L.tileLayer('https://tile.openstreetmap.org/{z}/{x}/{y}.png', {
        maxZoom: 19,
        attribution: '&copy; <a href="http://www.openstreetmap.org/copyright">OpenStreetMap</a>'
    }).addTo(map);

    return map;
}
