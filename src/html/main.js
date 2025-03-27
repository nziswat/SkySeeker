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
function receiveSignal(map, ID, lat, long, head, alt, speed) {

    // If it does not exist, create a new aircraft object
    if (hashMap.get(ID) === undefined) {

        // If lat and long are not defined, do not create a moving marker or polyline
        // Therefore, initialize them as undefined
        movingMarker = undefined;
        polyline = undefined;

        // Check if lat and long are defined
        if ((lat !== undefined) && (long !== undefined)) {
            // Initialize moving marker
            movingMarker = L.Marker.movingMarker(
                [[plane.lat, plane.long]], [0], { icon: planeIcon }
            ).addTo(map);

            // Create a polyline to track the plane's path
            polyline = L.polyline([], { color: 'blue' }).addTo(map);
        }
        
        // Create new aircraft object
        newAircraft = new Aircraft(ID, lat, long, head, alt, speed, polyline, movingMarker);

        // Insert it into the hash map
        hashMap.set(ID, newAircraft);
    }
    // If it does exist, update the aircraft object
    else {
        existingAircraft = hashMap.get(ID);
        properties = { lat, long, head, alt, speed };
        for (let key in properties) {
            if (properties[key] !== undefined) {
                existingAircraft[key] = properties[key];  // Correct property update
            }
        }

        // Update the movingMarker and polyline
        if ((lat !== undefined) && (long !== undefined)) {
            
            // If moving marker and polyline are undefined, initialize them
            if (existingAircraft.movingMarker === undefined) {
                // Initialize moving marker
                existingAircraft.movingMarker = L.Marker.movingMarker(
                    [[plane.lat, plane.long]], [0], { icon: planeIcon }
                ).addTo(map);

                // Create a polyline to track the plane's path
                existingAircraft.polyline = L.polyline([], { color: 'blue' }).addTo(map);
            } 
            // Otherwise, just update the movingMarker and polyline like normal
            else {
                existingAircraft.movingMarker.setLatLng([plane.lat, plane.long]);
                existingAircraft.polyline.addLatLng([lat, long]);
            }
        }
        if ((head !== undefined) && (existingAircraft.movingMarker !== undefined)) {
            existingAircraft.movingMarker.setRotationAngle(plane.head - 45); // -45 rotate for given icon
        }

        // Update the table with the current information
        //updateTable(existingAircraft);
    }

}

// Initialize the hash map to store aircrafts
let hashMap = new Map();
