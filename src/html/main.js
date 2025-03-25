// Main script for SkySeeker

// Function to initialize the map
function initializeMap(containerId, center, zoom) {

    var options = {};
    options.worldCopyJump = true; // Makes the markers repeat

    // Initialize the map
    const map = L.map(containerId, options).setView(center, zoom);

    // Add a tile layer
    const tiles = L.tileLayer('https://tile.openstreetmap.org/{z}/{x}/{y}.png', {
        maxZoom: 19, // Limits how far user can zoom in
        minZoom: 3,  // Limits how far user can zoom out
        attribution: '&copy; <a href="http://www.openstreetmap.org/copyright">OpenStreetMap</a>'
    }).addTo(map);

    return map;
}

// Function to update Aircraft/flight info
function recieveSignal(map, ID, lat, long, head, alt, speed) {

    // If it does not exist, create a new aircraft object
    if (hashMap.get(ID) === undefined) {

        // Initialize moving marker
        const movingMarker = L.Marker.movingMarker(
            [[plane.lat, plane.long]], [0], { icon: planeIcon }
        ).addTo(map);

        // Create a polyline to track the plane's path
        const polyline = L.polyline([], { color: 'blue' }).addTo(map);
        
        // Create new aircraft object
        let newAircraft = new Aircraft(ID, lat, long, head, alt, speed, polyline, movingMarker);

        // Insert it into the hash map
        hashMap.set(ID, newAircraft);
    }
    // If it does exist, update the aircraft object
    else {
        let existingAircraft = hashMap.get(ID);
        existingAircraft.lat = lat;
        existingAircraft.long = long;
        existingAircraft.head = head;
        existingAircraft.alt = alt;
        existingAircraft.speed = speed;

        // Update the movingMarker and polyline
        existingAircraft.movingMarker.setLatLng([plane.lat, plane.long]);
        existingAircraft.movingMarker.setRotationAngle(plane.head - 45); // -45 rotate for given icon
        existingAircraft.polyline.addLatLng([lat, long]);

        // Update the table with the current information
        //updateTable(existingAircraft);
    }

}

// Initialize the hash map to store aircrafts
let hashMap = new Map();
