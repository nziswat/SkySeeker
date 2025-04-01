// Aircraft Class
// Properties: ID, Latitude, Longitude, Even lat, Even long, Odd lat, Odd long,
// Heading, Altitude, Speed, moving marker, polyline, Even, Odd, Parity of last latlong signal

class Aircraft { 

    // Constructor function to build Aircraft object
    constructor(ID) {
        this._ID = ID;
    }

    // Getters and Setters

    // ID
    set ID(ID) { this._ID = ID; }
    get ID() { return this._ID; }

    // Latitude
    set lat(lat) { this._lat = lat; }
    get lat() { return this._lat; }

    // Longitude
    set long(long) { this._long = long; }
    get long() { return this._long; }

    // Even Latitude
    set e_lat(e_lat) { this._e_lat = e_lat; }
    get e_lat() { return this._e_lat; }

    // Even Longitude
    set e_long(e_long) { this._e_long = e_long; }
    get e_long() { return this._e_long; }

    // Odd Latitude
    set o_lat(o_lat) { this._o_lat = o_lat; }
    get o_lat() { return this._o_lat; }

    // Odd Longitude
    set o_long(o_long) { this._o_long = o_long; }
    get o_long() { return this._o_long; }

    // Heading
    set head(head) { this._head = head; }
    get head() { return this._head; }

    // Altitude
    set alt(alt) { this._alt = alt; }
    get alt() { return this._alt; }

    // Speed
    set speed(speed) { this._speed = speed; }
    get speed() { return this._speed; }

    // Moving marker
    set movingMarker(movingMarker) { this._movingMarker = movingMarker; }
    get movingMarker() { return this._movingMarker; }

    // Polyline
    set polyline(polyline) { this._polyline = polyline; }
    get polyline() { return this._polyline; }

    // Even
    set even(even) { this._even = even; }
    get even() { return this._even; }

    // Odd
    set odd(odd) { this._odd = odd; }
    get odd() { return this._odd; }

    // Parity of newest signal, false for even, true for odd
    set newestIsOdd(newestIsOdd) { this._newestIsOdd = newestIsOdd; }
    get newestIsOdd() { return this._newestIsOdd; }


    // https://airmetar.main.jp/radio/ADS-B%20Decoding%20Guide.pdf
    // Method to update the latitude and longitude
    updateLatLong(raw_lat, raw_long, fflag) {

        // Check if the signal is even or odd

        // If the signal is even, update the even boolean to true and update even long and lat
        if (fflag == 0) {
            this.even = true;
            this.e_lat = raw_lat / 131072; // 131072 is max value (17^2)
            this.e_long = raw_long / 131072;
            this.newestIsOdd = false;
        }
        // If the signal is odd, update the odd boolean to true and update odd long and lat
        else {
            this.odd = true;
            this.o_lat = raw_lat / 131072;
            this.o_long = raw_long / 131072;
            this.newestIsOdd = true;
        }
        // If you have both parts of the signal, calculate the lat and long
        if (this.even && this.odd) {
            // If the calculation goes through, reset the booleans
            if (this.calculateLatLong()) {
                this.odd = false;
                this.even = false;
            }           
        }
    }

    // Calculate the longitude and latitude
    calculateLatLong() {

        // Calculate latitude

        // Calculate latitude index j
        let j = Math.floor(59 * this.e_lat - 60 * this.o_lat + 0.5)
        let de_lat = 6;
        let do_lat = (360 / 59);
        this.e_lat = de_lat * ((j % 60) + this.e_lat);
        this.o_lat = do_lat * ((j % 59) + this.o_lat);

        if (this.e_lat >= 270) {
            this.e_lat -= 360;
        }
        if (this.o_lat >= 270) {
            this.o_lat -= 360;
        }
        let e_NL = this.NL(this.e_lat);
        let o_NL = this.NL(this.o_lat);

        // If the NL functions don't match, the signals are from different points, quit calculation
        if (e_NL != o_NL) {
            return false;
        }

        let calculated_lat;

        // Choose the newest latitude
        if (this.newestIsOdd) {
            calculated_lat = this.o_lat;
        } else {
            calculated_lat = this.e_lat;
        }

        // Calculate longitude
        let calculated_long;
        // If the newest data is odd, do this calculation
        if (this.newestIsOdd) {
            let n = Math.max(o_NL - 1, 1)
            let DLong = 360 / n
            let m = Math.floor(this.e_long * (o_NL - 1) - this.o_long * o_NL + 0.5);
            calculated_long = DLong * ((m % n) + this.o_long);

            if (calculated_long >= 180) {
                calculated_long -= 360;
            }
        }
        // If the newest data is even, do this calculation
        else {
            let n = Math.max(e_NL, 1)
            let DLong = 360 / n
            let m = Math.floor(this.e_long * (e_NL - 1) - this.o_long * e_NL + 0.5);
            calculated_long = DLong * ((m % n) + this.e_long);

            if (calculated_long >= 180) {
                calculated_long -= 360;
            }
        }

        // Update the lat and long properties
        this.lat = calculated_lat;
        this.long = calculated_long;

        // Return true since you successfully calculated the lat long
        return true;
    }

    // Calculate the number of longitude zones
    NL(lat) {
        let a = 1 - Math.cos(Math.PI / (30))
        let b = Math.cos((Math.PI/180) * lat)**2
        let c = Math.acos(1 - a/b)
        return Math.floor(( 2 * Math.PI )/c)
    } 
}

// Function to update Aircraft/flight info
function receiveSignal(map, ID, lat, long, head, alt, speed, fflag) {

    // If it does not exist, create a new aircraft object
    if (hashMap.get(ID) == undefined) {

        // Create new aircraft object
        let newAircraft = new Aircraft(ID);

        // Insert it into the hash map
        hashMap.set(ID, newAircraft);

        // Check if lat and long are defined
        if ((lat != undefined) && (long != undefined)) {
            newAircraft.updateLatLong(lat, long, fflag);
        }
        console.log("Updated Aircraft Data3:", "icao:", newAircraft.ID, "lat:", newAircraft.lat, "long:", newAircraft.long,
            "head:", newAircraft.head, "alt:", newAircraft.alt, "speed:", newAircraft.speed);

    }
    // If it does exist, update the aircraft object
    else {
        let existingAircraft = hashMap.get(ID);
        let properties = { head, alt, speed };
        for (let key in properties) {
            if (properties[key] != undefined) {
                existingAircraft[key] = properties[key];  // Correct property update
            }
        }

        // Check if lat and long are defined
        if ((lat != undefined) && (long != undefined)) {
            // Try to update them
            existingAircraft.updateLatLong(lat, long, fflag);
        }


        // Update the movingMarker and polyline
        if ((existingAircraft.lat != undefined) && (existingAircraft.long != undefined)) {

            // If moving marker and polyline are undefined, initialize them
            if (existingAircraft.movingMarker == undefined) {
                // Initialize moving marker if heading is known
                if (head != undefined) {
                    existingAircraft.movingMarker = L.Marker.movingMarker(
                        [[existingAircraft.lat, existingAircraft.long]], [0], { icon: planeIcon }
                    ).addTo(map);
                }
                // Create a polyline to track the plane's path
                existingAircraft.polyline = L.polyline([], { color: 'blue' }).addTo(map);
            }
            // Otherwise, just update the movingMarker and polyline like normal
            else {
                existingAircraft.movingMarker.setLatLng([existingAircraft.lat, existingAircraft.long]);
                existingAircraft.polyline.addLatLng([existingAircraft.lat, existingAircraft.long]);
            }
        }
        if ((head != undefined) && (existingAircraft.movingMarker != undefined)) {
            existingAircraft.movingMarker.setRotationAngle(existingAircraft.head - 45); // -45 rotate for given icon
        }

        console.log("Updated Aircraft Data3:", "icao:", existingAircraft.ID, "lat:", existingAircraft.lat, "long:", existingAircraft.long,
            "head:", existingAircraft.head, "alt:", existingAircraft.alt, "speed:", existingAircraft.speed);

        // Update the table with the current information
        //updateTable(existingAircraft);
    }
        
}

// Initialize the hash map to store aircrafts
let hashMap = new Map();
