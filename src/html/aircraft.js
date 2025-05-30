// Aircraft Class
// Properties: ID, Latitude, Longitude, Even lat, Even long, Odd lat, Odd long,
// Heading, Altitude, Speed, moving marker, polyline, Even, Odd, Parity of last latlong signal

class Aircraft { 

    // Constructor function to build Aircraft object
    constructor(ID) {
        this._ID = ID;
        this._saved = false; // saved to database
        this.ghostTimeout = null;
        this.ghostTimeout = setTimeout(() => this.ghostAircraft(), 6000);
        this.deleteTimeout = null;
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


    // Save last calculated lat and long in case of overriding incorrect first position
    set prev_lat(prev_lat) { this._prev_lat = prev_lat; }
    get prev_lat() { return this._prev_lat; }

    set prev_long(prev_long) { this._prev_long = prev_long; }
    get prev_long() { return this._prev_long; }

    // saved
    set saved(saved) { this._saved = saved; }
    get saved() { return this._saved; }

    // model
    set model(model) { this._model = model; }
    get model() { return this._model; }

    // country of origin
    set country(country) { this._country = country; }
    get country() { return this._country; }




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
        let new_e_lat = this.e_lat
        let new_o_lat = this.o_lat
        let new_e_long = this.e_long
        let new_o_long = this.o_long
        new_e_lat = de_lat * ((j % 60) + new_e_lat);
        new_o_lat = do_lat * ((j % 59) + new_o_lat);

        if (new_e_lat >= 270) {
            new_e_lat -= 360;
        }
        if (new_o_lat >= 270) {
            new_o_lat -= 360;
        }
        let e_NL = this.NL(new_e_lat);
        let o_NL = this.NL(new_o_lat);

        // If the NL functions don't match, the signals are from different points, quit calculation
        if (e_NL != o_NL) {
            return false;
        }

        let calculated_lat;

        // Choose the newest latitude
        if (this.newestIsOdd) {
            calculated_lat = new_o_lat;
        } else {
            calculated_lat = new_e_lat;
        }

        // Calculate longitude
        let calculated_long;
        // If the newest data is odd, do this calculation
        if (this.newestIsOdd) {
            let n = Math.max(o_NL - 1, 1)
            let DLong = 360 / n
            let m = Math.floor(new_e_long * (o_NL - 1) - new_o_long * o_NL + 0.5);
            calculated_long = DLong * ((m % n) + new_o_long);

            if (calculated_long >= 180) {
                calculated_long -= 360;
            }
        }
        // If the newest data is even, do this calculation
        else {
            let n = Math.max(e_NL, 1)
            let DLong = 360 / n
            let m = Math.floor(new_e_long * (e_NL - 1) - new_o_long * e_NL + 0.5);
            calculated_long = DLong * ((m % n) + new_e_long);

            if (calculated_long >= 180) {
                calculated_long -= 360;
            }
        }

        // Update the lat and long properties

        /* // Code to only show plane if you get two correct lat longs, commented out because its too slow
        if ((this.lat == undefined) || (this.long == undefined)) { // If undefined, dont update until you get 2 similar in a row

            if (Math.abs(this.prev_lat - calculated_lat) <= 1 && Math.abs(this.prev_long - calculated_long) <= 1) { // If you recieve two signals in a row are close, update the lat long
                this.lat = calculated_lat;
                this.long = calculated_long;
            }

        }        
        */

        if ((this.lat == undefined) || (this.long == undefined)){ // Just update if undefined
            this.lat = calculated_lat;
            this.long = calculated_long;
        }        
        else if (Math.abs(this.lat - calculated_lat) <= 1 && Math.abs(this.long - calculated_long) <= 1) { // if the new coords are not more than 1 value greater, it is probably not an error
            this.lat = calculated_lat;
            this.long = calculated_long;
        }
        else if (Math.abs(this.prev_lat - calculated_lat) <= 1 && Math.abs(this.prev_long - calculated_long) <= 1) { // If you recieve two signals in a row are far from current but close to each other, the first lat long was likely incorrect, so overrwrite it
            console.log(`Overriding incorrect coords; Significant change detected for ${this.ID}: lat(${this.lat} -> ${calculated_lat}), long(${this.long} -> ${calculated_long})\n
            even values: ${this.e_lat}, ${this.e_long}\todd values: ${this.o_lat}, ${this.o_long}`);
            this.lat = calculated_lat;
            this.long = calculated_long;

            // Reset incorrect polyline coordinate(s)
            if (this.polyline) {
                this.polyline.setLatLngs([]);
            }
            
        }
        else { // if the new coords are more than 1 value greater, it is probably an error
            console.log(`Significant change detected for ${this.ID}: lat(${this.lat} -> ${calculated_lat}), long(${this.long} -> ${calculated_long})\n
            even values: ${this.e_lat}, ${this.e_long}\todd values: ${this.o_lat}, ${this.o_long}`);
        }

        // Save the calcuated values in case of overriding incorrect first position
        this.prev_lat = calculated_lat;
        this.prev_long = calculated_long;

        // Return true since you successfully calculated the lat long, even if they weren't actually updated 
        return true;
    }

    /* The NL function uses the precomputed table from 1090-WP-9-14 */
    NL(lat) {
        if (lat < 0) lat = -lat; /* Table is symmetric about the equator. */
        if (lat < 10.47047130) return 59;
        if (lat < 14.82817437) return 58;
        if (lat < 18.18626357) return 57;
        if (lat < 21.02939493) return 56;
        if (lat < 23.54504487) return 55;
        if (lat < 25.82924707) return 54;
        if (lat < 27.93898710) return 53;
        if (lat < 29.91135686) return 52;
        if (lat < 31.77209708) return 51;
        if (lat < 33.53993436) return 50;
        if (lat < 35.22899598) return 49;
        if (lat < 36.85025108) return 48;
        if (lat < 38.41241892) return 47;
        if (lat < 39.92256684) return 46;
        if (lat < 41.38651832) return 45;
        if (lat < 42.80914012) return 44;
        if (lat < 44.19454951) return 43;
        if (lat < 45.54626723) return 42;
        if (lat < 46.86733252) return 41;
        if (lat < 48.16039128) return 40;
        if (lat < 49.42776439) return 39;
        if (lat < 50.67150166) return 38;
        if (lat < 51.89342469) return 37;
        if (lat < 53.09516153) return 36;
        if (lat < 54.27817472) return 35;
        if (lat < 55.44378444) return 34;
        if (lat < 56.59318756) return 33;
        if (lat < 57.72747354) return 32;
        if (lat < 58.84763776) return 31;
        if (lat < 59.95459277) return 30;
        if (lat < 61.04917774) return 29;
        if (lat < 62.13216659) return 28;
        if (lat < 63.20427479) return 27;
        if (lat < 64.26616523) return 26;
        if (lat < 65.31845310) return 25;
        if (lat < 66.36171008) return 24;
        if (lat < 67.39646774) return 23;
        if (lat < 68.42322022) return 22;
        if (lat < 69.44242631) return 21;
        if (lat < 70.45451075) return 20;
        if (lat < 71.45986473) return 19;
        if (lat < 72.45884545) return 18;
        if (lat < 73.45177442) return 17;
        if (lat < 74.43893416) return 16;
        if (lat < 75.42056257) return 15;
        if (lat < 76.39684391) return 14;
        if (lat < 77.36789461) return 13;
        if (lat < 78.33374083) return 12;
        if (lat < 79.29428225) return 11;
        if (lat < 80.24923213) return 10;
        if (lat < 81.19801349) return 9;
        if (lat < 82.13956981) return 8;
        if (lat < 83.07199445) return 7;
        if (lat < 83.99173563) return 6;
        if (lat < 84.89166191) return 5;
        if (lat < 85.75541621) return 4;
        if (lat < 86.53536998) return 3;
        if (lat < 87.00000000) return 2;
        else return 1;
    }
    /* Actual calculation, not in use because it takes a lot more processing than using a table
    // Calculate the number of longitude zones
    NL(lat) {
        let a = 1 - Math.cos(Math.PI / (30))
        let b = Math.cos((Math.PI/180) * lat)**2
        let c = Math.acos(1 - a/b)
        return Math.floor(( 2 * Math.PI )/c)
    } */
    //function to call to database to get more info on aircraft (if possible)
    checkICAOData() {
    let query_string = `getICAOData${this.ID}`;
        window.cefQuery({
            request: query_string,
            onSuccess: (response) => {
                let parse = JSON.parse(response);
                this.model = parse.typeCode;
                this.country = parse.country;
            },
            onFailure: (error_code, error_message) => {
                console.error("Query failed", error_code, error_message);
            }
        });
    }
    //save aircraft to DB
    save() {
        console.log(`Saving ${this.ID}`);
        let fixlat = formatToSixChars(this.lat);
        let fixlong = formatToSixChars(this.long);
        let query_string = `savAircraft${this.ID}${fixlat}${fixlong}`;
        window.cefQuery({
            request: query_string,
            onSuccess: (response) => {
                this.saved = true;
                this.movingMarker.setIcon(savedIcon);
            },
            onFailure: function (error_code, error_message) {
                console.error("Aircraft failed to save!", error_code, error_message);
            }
        });
    }
    // check if aircraft is in database
    checkSaved() {
        let query_string = `lodAircraft${this.ID}`;
        window.cefQuery({
            request: query_string,
            onSuccess: (response) => {
                console.log(`Aircraft ${this.ID} loaded.`);
                this.saved = true;
            },
            onFailure: function (error_code, error_message) {
                //console.error("Aircraft not found", error_code, error_message);
            }
        });


    }
    //function to ghost aircraft after 60 seconds
    ghostAircraft() {
        if (this.movingMarker) { 
            this.movingMarker.setOpacity(0.25);
        }
        this.deleteTimeout = setTimeout(() => this.deleteAircraft(), 60000);

    }
    //interrupt timer
    interruptTimer() {
        clearTimeout(this.ghostTimeout); // clear first timeout and reset
        this.ghostTimeout = setTimeout(() => this.ghostAircraft(), 60000);
        if (this.movingMarker) {
            this.movingMarker.setOpacity(1);
        }
        clearTimeout(this.deleteTimeout);
    }
    deleteAircraft() {
        vaporize(this.ID);
        hashMap.delete(this.ID);
        if (this.movingMarker) {
            this.movingMarker.remove();
        }
        if (this.polyline) {
            this.polyline.remove();
        }
    }



}

// minor helper function to force 6 chars (for query)
function formatToSixChars(num) {
    let str = num.toFixed(6);          
    str = parseFloat(str).toString();  
    if (str.length > 6) {
        return str.slice(0, 6);        
    } else {
        return str.padEnd(6, '0');     
    }
}
