//////////////////////////////////////////////////////////////////////
// ---------------------- Copyright notice ---------------------------
// This source code is part of the EVASoft project
// It is property of Alain Boute Ingenierie - www.abing.fr and is
// distributed under the GNU Public Licence version 2
// Commercial use is submited to licencing - contact eva@abing.fr
// -------------------------------------------------------------------
//        File : geopos.js
// Description : handling functions for geographical positions
//      Author : Alain BOUTE
//     Created : Dec 20 2016
//////////////////////////////////////////////////////////////////////

var geopos_pho, geopos_lat, geopos_lng;

// Convert position HMSD to decimal
function imgPos(dat, ll) {   
	var p = EXIF.getTag(dat, "GPS" + ll);
	if(p == null) { return ''; }
	var d = EXIF.getTag(dat, "GPS" + ll + "Ref");
    return (p[0] + p[1]/60 + p[2]/3600) * ((d == "S" || d == "W") ? -1 : 1);
}

// Fill lat and lng inputs whith current GPS position
function setLatLngFromGPS(lat,lng) {
	if(!navigator.geolocation) return;
	navigator.geolocation.getCurrentPosition(function (e) {
		var p = e.coords.latitude.toString().substring(0,8);
		if(p == '') return;
		lat.value = p;
		lng.value = e.coords.longitude.toString().substring(0,8);
	});
}

// Define lat and lng inputs whith current GPS position
function defLatLngFromGPS(name_pfx) {
	if(!navigator.geolocation) return;
	navigator.geolocation.getCurrentPosition(function (e) {
		var p = e.coords.latitude.toString().substring(0,8);
		if(p == '') return;
		geopos_lat.value = p;
		geopos_lng.value = e.coords.longitude.toString().substring(0,8);
	});
}

// Fill lat and lng inputs whith GPS position of photo file being loaded in n_pho input
function setLatLngFromPhoto(n_pho,n_lat,n_lng) {
	geopos_lat = document.getElementsByName(n_lat + "$1")[0];
	geopos_lng = document.getElementsByName(n_lng + "$1")[0];
	if(geopos_lat == null || geopos_lng == null) return;
	geopos_pho = document.getElementsByName(n_pho + "$1")[0];
	if(geopos_lat.value == "" || geopos_lat.value == "") setLatLngFromGPS(geopos_lat, geopos_lng);
	if(geopos_pho != null) geopos_pho.onchange = function (e) {
		EXIF.getData(e.target.files[0], function () {
				var p = imgPos(this, "Latitude").toString().substring(0,8);
				if(p == '') return;
				geopos_lat.value = p;
				geopos_lng.value = imgPos(this, "Longitude").toString().substring(0,8);
			});
	};
}
