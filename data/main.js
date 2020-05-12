function reformatDate(date){
	var year = date.substring(0,4);
	var month = date.substring(5,7);
	var day = date.substring(8,10);
	var hour = parseInt(date.substring(11,13));
	var minute = date.substring(14,16);
	var ampm = "am";
	if(hour > 12){
		hour -= 12;
		ampm = "pm";
	}
	
	return hour + ":" + minute + ampm + " " + month + "/" + day + "/" + year;
}

function reformatDate2(date){
	var year = date.substring(0,4);
	var month = date.substring(5,7);
	var day = date.substring(8,10);
	
	return month + "/" + day + "/" + year;
}

function reformatTime(time){
	var hour = time.substring(0,2);
	var minute = time.substring(3,5);
	var ampm = "am";
	if(hour > 12){
		hour -= 12;
		ampm = "pm";
	}
	return hour + ":" + minute + ampm;
}


function graph(id, range, key, precision, array){
	document.getElementById(id).innerHTML = "";

	var max;
	var min;
	var r;
	//address if the array is under filled
	if(array.length == 0){
		max = 0;
		min = 0;
		r = 0;
	}else{
		max = array[0][key];
		min = array[0][key];
	}
	
	array.forEach(function(value, index, array){
		var v = value[key];
		if(v > max){
			max = v;
		}
		if(v < min){
			min = v;
		}
	});
	r = max - min;
	
	var spans = document.querySelectorAll("#"+range + " span");
	spans[0].innerHTML = min.toFixed(precision);
	spans[1].innerHTML = max.toFixed(precision);
	spans[2].innerHTML = r.toFixed(precision);
	
	for(var i=array.length; i < 24; i++){
		var div = document.createElement("div");
		div.setAttribute('style','height: 1px; margin-top: -69px');
		document.getElementById(id).appendChild(div);
	}
	
	array.forEach(function(value, index, array){
		var v = value[key];
		var div = document.createElement("div");
		var h = (v - min) / r * 70;
		var m = 70-h;
		if(h == 0){
			h = 1;
			m = 69;
		}
		div.setAttribute('style','height: ' + h + 'px; margin-top: -' + m + 'px');
		document.getElementById(id).appendChild(div);
	});
}

function table(array){
	var t = document.querySelector("table tbody");
	t.innerHTML = "";

	array.forEach(function(value, index, array){
		var tr = document.createElement("tr");
		var date_td = document.createElement("td");
		var time_td = document.createElement("td");
		var temp_td = document.createElement("td");
		var humidity_td = document.createElement("td");
		var pressure_td = document.createElement("td");
		
		date_td.innerHTML = reformatDate2(value.time);
		time_td.innerHTML = reformatTime(value.time.substring(11,16));
		temp_td.innerHTML = value.temperature;
		humidity_td.innerHTML = value.humidity;
		pressure_td.innerHTML = value.barometer;

		tr.appendChild(date_td);
		tr.appendChild(time_td);
		tr.appendChild(temp_td);
		tr.appendChild(humidity_td);
		tr.appendChild(pressure_td);

		t.appendChild(tr);
	});
	
}

function ajax(){
	var request = new XMLHttpRequest();
	request.responseType = "json";
	request.addEventListener("load", function(){
		//Current Conditions
		document.getElementById("time").innerHTML = reformatDate(request.response.current.time);
		document.getElementById("station").innerHTML = request.response.station;
		
		document.getElementById("temperature").innerHTML = request.response.current.temperature;
		document.getElementById("humidity").innerHTML = request.response.current.humidity;
		document.getElementById("barometer").innerHTML = request.response.current.barometer;
		document.getElementById("barometer_range").innerHTML = request.response.current.pressure;
		document.getElementById("trend").innerHTML = request.response.current.trend;
		
		var img;
		switch(request.response.current.trend){
			case "rising":
				img = "up.png";
				break;
			case "falling":
				img = "down.png";
				break;
			case "rapidly rising":
				img = "big_up.png";
				break;
			case "rapidly falling":
				img = "big_down.png";
				break;
			case "steady":
				img = "steady.png";
				break;
			default:
				img = "unknown.png";
		}
		document.getElementById("trend_img").src = img;
		
		//Record data
		document.getElementById("low_temperature").innerHTML = request.response.records.temperature.low;
		document.getElementById("high_temperature").innerHTML = request.response.records.temperature.high;
		
		document.getElementById("low_humidity").innerHTML = request.response.records.humidity.low;
		document.getElementById("high_humidity").innerHTML = request.response.records.humidity.high;
		
		document.getElementById("low_barometer").innerHTML = request.response.records.barometer.low;
		document.getElementById("high_barometer").innerHTML = request.response.records.barometer.high;
		
		//graph
		graph("graph_temperature","temperature_range", "temperature", 2,request.response.history);
		graph("graph_humidity","humidity_range", "humidity",2, request.response.history);
		graph("graph_barometer","barometer_range", "barometer",2, request.response.history);

		//Historical Data
		table(request.response.history);
		
		document.getElementById("main").style.display = "block";
		document.getElementById("loading").style.display = "none";

		//Set the next auto refesh if needed		
		setTimeout(function(){
			if(autoRefresh === true){
				ajax();
			}
		}, 60 * 1000 * 10);
	});
	
	request.addEventListener("error", function(){
		alert("Failed to get weather.");
	});
	
	request.open("GET", "json_no_frost?"+ new Date().getTime());
	request.send();
	
}

var autoRefresh;
window.onload = function(){
	ajax();

	if(localStorage['auto_refresh'] == "false"){
		autoRefresh = false;
		document.getElementById("auto_refresh").innerHTML = "Auto Refresh (Off)";
		document.getElementById("auto_refresh").classList.add("red");
		document.getElementById("auto_refresh").classList.remove("gray");
	}else{
		document.getElementById("auto_refresh").innerHTML = "Auto Refresh (On)";
		autoRefresh = true;
	}
	
	document.getElementById("auto_refresh").addEventListener('click', function(){
		if(localStorage['auto_refresh'] == "false"){
			document.getElementById("auto_refresh").innerHTML = "Auto Refresh (On)";
			localStorage['auto_refresh'] = "true";
			document.getElementById("auto_refresh").classList.add("gray");
			document.getElementById("auto_refresh").classList.remove("red");
		}else{
			document.getElementById("auto_refresh").innerHTML = "Auto Refresh (Off)";
			localStorage['auto_refresh'] = "false";
			document.getElementById("auto_refresh").classList.add("red");
			document.getElementById("auto_refresh").classList.remove("gray");
		}
		console.log("TEST");
	});

	document.getElementById("reset_record").addEventListener('click', function(){
		var x = confirm("Are you sure you want to reset record high/lows?");
		if(x){
			document.getElementById("main").style.display = "none";
			document.getElementById("loading").style.display = "block";
			var request = new XMLHttpRequest();
			request.responseType = "json";
			request.addEventListener("load", function(){
				ajax();
			});

			request.addEventListener("error", function(){
				alert("Failed to reset records.");
			});
	
			request.open("POST", "json_reset_record");
			request.send();
		}
	});

	document.getElementById("reset_historical").addEventListener('click', function(){
		var x = confirm("Are you sure you want to reset historical data?");
		if(x){
			document.getElementById("main").style.display = "none";
			document.getElementById("loading").style.display = "block";
			var request = new XMLHttpRequest();
			request.responseType = "json";
			request.addEventListener("load", function(){
				ajax();
			});

			request.addEventListener("error", function(){
				alert("Failed to reset historical data.");
			});
	
			request.open("POST", "json_reset_history");
			request.send();
		}
	});

	document.getElementById("btn_settings").addEventListener('click', function(){
		document.getElementById("main").style.display = "none";
		document.getElementById("loading").style.display = "block";
		console.log(document.getElementById("main"));
		var request = new XMLHttpRequest();
		request.responseType = "json";
		request.addEventListener("load", function(){
			document.getElementById("txt_station").value = request.response.station;
			document.getElementById("txt_remote").value = request.response.remote;
			document.getElementById("time_time").value = request.response.time.substring(11,16);
			document.getElementById("date_date").value = request.response.time.substring(0,10);

			document.getElementById("settings").style.display = "block";
			document.getElementById("loading").style.display = "none";
		});

		request.addEventListener("error", function(){
			alert("Failed to get settings.");
		});

		request.open("POST", "json_get_settings");
		request.send();
	});

	document.getElementById("cancel").addEventListener('click', function(){
		document.getElementById("main").style.display = "block";
		document.getElementById("settings").style.display = "none";
	});

	document.getElementById("save").addEventListener('click', function(){
		document.getElementById("loading").style.display = "block";
		document.getElementById("settings").style.display = "none";

		var request = new XMLHttpRequest();
		request.responseType = "json";
		request.addEventListener("load", function(){
			ajax();
		});

		request.addEventListener("error", function(){
			alert("Failed to save settings.");
		});

		request.open("POST", "json_save_settings");

		//form data to send
		var obj = {
			"ssid":document.getElementById("txt_ssid").value,
			"password":document.getElementById("txt_password").value,
			"remote":document.getElementById("txt_remote").value,
			"station":document.getElementById("txt_station").value,
			"date":document.getElementById("date_date").value,
			"time":document.getElementById("time_time").value
		}
		var query = "";
		request.send(query);
	});


}