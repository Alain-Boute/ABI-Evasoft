/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact postmaster@abing.fr
** -------------------------------------------------------------------
**        File : gmaps.js
** Description : simple interface for Google Maps API V3
**      Author : Alain BOUTE
**     Created : Mar 6 2014
*********************************************************************/
var gmap_map = 0;
var gmap_bounds = new google.maps.LatLngBounds();
var gmap_geocoder = new google.maps.Geocoder();
var gmap_infwin = new google.maps.InfoWindow();
var gmap_dir_svc = new google.maps.DirectionsService();
var gmap_nbqry = 0, gmap_nbres = 0, gmap_nbmrk = 0, gmap_nbbnd = 0, gmap_nberr = 0, gmap_tot = 0, gmap_err = '';
var gmap_addresses = new Array(), gmap_markers = new Array();
var gmap_pos1 = 0, gmap_pos2 = 0, gmap_idform=0, gmap_idctrl=0, gmap_openform = 0;
var gmap_pos_input = [], gmap_lng_input = [];
var gmap_zoom = 14, gmap_disptype = google.maps.MapTypeId.ROADMAP;
var gmap_init_cb = null;

// Display process results in error block
function gmap_display_res() {
//console.debug('gmap_display_res : gmap_nbmrk = ' + gmap_nbmrk);
	var el = document.getElementById('gmap_err');
	if(el) {
		var nb_vis = 0, bounds = gmap_map.getBounds();
		for(var i = 0; i < gmap_markers.length && gmap_nbbnd; i++) {
			if(!(bounds === undefined) && gmap_markers[i].getVisible() && bounds.contains(gmap_markers[i].getPosition())) { nb_vis++; } }
		el.innerHTML = '<p align=center><big><big>' + nb_vis + ' / ' + gmap_nbmrk + ' marqueurs affichés</big>' +
								((gmap_nbqry - gmap_nbres) > 0 ? ' - à traiter : ' + (gmap_nbqry - gmap_nbres) : '') + '</big></p>' + 
								(gmap_err == '' ? '' : ('<u>Liste des erreurs (' + gmap_nberr + ') :</u><ul>' + gmap_err + '</ul>'));
	}
}

// Display all markers by changing bounds
function gmap_showall(b_all) {
	if(b_all) {
		var bounds = gmap_map.getBounds();
		for(var i = 0; i < gmap_markers.length; i++) { bounds.extend(gmap_markers[i].getPosition()); }
		gmap_map.fitBounds(bounds);
	} else {
		gmap_map.fitBounds(gmap_bounds);
	}
}

// Create new marker from position
function gmap_add_marker(pos,adr,ville,nom,lien,txt,ico,b_bounds,idobj,iadr) {
	// Handle markers counter
	gmap_nbmrk++;

	// Handle EVA fields for draggable marker
	var gmap_bdrag = false;
	if(b_bounds >= 2)
	{
		gmap_bdrag = true;

		// Search for single EVA input with GMAPS_SEARCH field
		gmap_pos_input = document.getElementsByName('D' + gmap_idctrl + '/' + gmap_idform + '$' + idobj + '#GMAPS_SEARCH$1')
		if(!gmap_pos_input.length)
		{
			// Search for two EVA inputs with LAT_LIEU & LNG_LIEU fields
			gmap_pos_input = document.getElementsByName('D' + gmap_idctrl + '/' + gmap_idform + '$' + idobj + '#LAT_LIEU$1');
			gmap_lng_input = document.getElementsByName('D' + (gmap_idctrl+1) + '/' + gmap_idform + '$' + idobj + '#LNG_LIEU$1');

			// Disable draggable if no storage field found
			if(!gmap_pos_input.length || !gmap_lng_input.length) gmap_bdrag = false;
		}
	}

	// Initialize map on first call
	if(!gmap_map) {
		var opt= { center: pos, zoom: gmap_zoom, mapTypeId: gmap_disptype };
		gmap_map = new google.maps.Map(document.getElementById('gmap'), opt);
		if(gmap_init_cb != null) gmap_init_cb();
	}

	// Create marker
	var marker = new google.maps.Marker({ map: gmap_map, position: pos, icon: ico, title: nom, draggable: gmap_bdrag });
	gmap_markers.push(marker);
	google.maps.event.addListener(marker, 'click', function() {
			gmap_infwin.open(gmap_map, marker);
			gmap_infwin.setContent('<p><big><b>' + lien + '</b></big>' + (txt ? '<br>' : '') + txt + 
									(adr == '' ? '' : ('<br><small>' + adr + '</small>') +
									(ville == '' ? '' : ('<br><b>' + ville + '</b>'))) + '</p>');
		});

	// Handle storage of marker position (draggable marker)
	if(gmap_bdrag)
	{
		// Handle input with GMAPS_SEARCH field
		if(!gmap_lng_input.length) {
			google.maps.event.addListener(marker, 'dragend',
				function (e) { gmap_pos_input[0].value = e.latLng.toUrlValue(); });
		}
		else
		{
			// Handle inputs with LAT_LIEU & LNG_LIEU fields
			google.maps.event.addListener(marker, 'dragend',
				function (e)
				{ 
					var url = e.latLng.toUrlValue();
					var p = url.indexOf(',');
					gmap_pos_input[0].value = url.substring(0, p);
					gmap_lng_input[0].value = url.substring(p + 1);
				});
		}
	}

	// Handle map bounds
	if(b_bounds > 0) {
		gmap_bounds.extend(pos);
		if(gmap_nbbnd++) { gmap_map.fitBounds(gmap_bounds); }

		// Handle trip calculation when 2 positions are marked
		if(b_bounds == 1 && gmap_pos2 == 0) gmap_pos2 = pos;
		if(b_bounds == 2 && gmap_pos1 == 0) gmap_pos1 = pos;
		if(gmap_pos1 && gmap_pos2) {
			var request = {	origin:gmap_pos1, destination:gmap_pos2, travelMode: google.maps.TravelMode.DRIVING };
			gmap_dir_svc.route(request, function(result, status) {
				if (status == google.maps.DirectionsStatus.OK) {
					var gmap_dir_dsp = new google.maps.DirectionsRenderer( { suppressMarkers: true, draggable: true } );
					gmap_dir_dsp.setDirections(result);
					gmap_dir_dsp.setMap(gmap_map);
					var el = document.getElementById('gmap_routeinfo');
					if(el) { el.innerHTML = 'Longueur du trajet : ' + result.routes[0].legs[0].distance.text + 
											'<br>Durée normale : ' + result.routes[0].legs[0].duration.text; }
				} });
		}
	}
	google.maps.event.addListener(gmap_map, 'bounds_changed', gmap_display_res);
	gmap_display_res();
}

// Create or geocode marker from address
function gmap_prepare(gpos,adr,ville,nom,lien,txt,ico,b_bounds,idobj,iadr) {
	var ipos = gpos.indexOf(",");
	var pos = new google.maps.LatLng(gpos.slice(0, ipos-1),gpos.slice(ipos+1, gpos.length));
	if(ipos  > 0 && !isNaN(pos.lat()) && !isNaN(pos.lng())) {
		gmap_add_marker(pos,adr,ville,nom,lien,txt,ico,b_bounds,idobj,iadr);
	} else {
		gmap_nbqry++;
		setTimeout(function() { gmap_geocoder.geocode( { address: (gpos != '' ? gpos : ((adr == '' ? '' : adr + ', ') + ville)) },
			function (results, status) {
				gmap_nbres++;
				if (status == google.maps.GeocoderStatus.OK) {
					if(results.length) {
						gmap_add_marker(results[0].geometry.location,adr,ville,nom,lien,txt,ico,b_bounds,idobj,iadr);
					}
					if(results.length == 1) {
						var el = document.getElementById('gmap_pos');
						if(el) el.innerHTML = el.innerHTML + idobj + ';' + results[0].geometry.location.toUrlValue(6) + ';' + nom + '<br>';
					} else if(results.length > 1) {
						gmap_err = gmap_err + '<li>Adresse approximative pour ' + lien + ' : ' + results.length + ' résultats</li>';
						gmap_nberr++;
					} else {
						gmap_err = gmap_err + '<li>Adresse non trouvée ' + lien + ' : ' + adr + ', ' + ville + '</li>';
						gmap_nberr++;
					}
				} else {
					gmap_err = gmap_err + '<li>Adresse non trouvée pour ' + lien + ': ' + status + '</li>';
					gmap_nberr++;
				}
			});}, b_bounds == 2 ? 10 : (1000 * gmap_nbqry));
	}
}

// Initialize map
function gmap_init() {
	// Add markers for adresses
	for(var i = 0; i < gmap_addresses.length; i++) {
		var e = gmap_addresses[i],gpos = e[0],adr = e[1],ville = e[2],nom = e[3],lien = e[4],txt = e[5],ico = e[6],b_bounds = e[7],idobj=e[8];
		gmap_prepare(gpos,adr,ville,nom,lien,txt,ico,b_bounds,idobj,i);
	}
}

// Query for new marker from address
function gmap_add_address(gpos,adr,ville,nom,idobj,txt,ico,b_bounds) {
	if(!gmap_addresses.length) { google.maps.event.addDomListener(window, 'load', gmap_init); }
	gmap_addresses.push(new Array(gpos,adr,ville,nom,'<a href=javascript:cb("B$#.OPENOBJ=' + idobj +
						(gmap_openform == 0 ? '' : (',' + gmap_openform + ',0,0')) + '")>' + nom + '</a>',txt,ico,b_bounds,idobj));
}

// Get EVA input not hidden
function EVA_get_input(eltname)
{
	var el = document.getElementsByName(eltname + "$1");
	if(el.length == 0) return null;
	var i;
	for(i=0; i < el.length; i++)
		if(el[i].type != "hidden") return el[i];
	return el[0];
}

// Reverse geocode a position
function gmap_revgeocode(lat,lng,adrname, ctyname)
{
	var a = document.getElementsByName(adrname + "$1")[0];
	if(a.value != "") return;
	gmap_geocoder.geocode({'location': {'lat':lat,'lng':lng}}, function(res, sts) {
    if (sts === google.maps.GeocoderStatus.OK) {
			var adr = res[0].formatted_address.split(", ",3);
			a.value = adr[0];
			var c = EVA_get_input(ctyname);
			var opt = c.options;
			var cp = adr[1].substr(0,5);
			var i;
			for(i=0; i >= 0 && i < opt.length; i++)
				if(opt[i].text.indexOf(cp) >= 0) {
					opt.selectedIndex = i; c.selectedIndex = i; i = -2; }
			if(i>=0) a.value = adr[0] + '\n' + adr[1];
		}});
}