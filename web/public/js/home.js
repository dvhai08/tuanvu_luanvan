

var name, email, photoUrl, uid, emailVerified;
var device_mac_key;
var listCurrentStatus;
var currentStatus;
var select = document.getElementById("select_devices");
var chkDeviceSTT1 = document.getElementById("chkDevice1");
var chkDeviceSTT2 = document.getElementById("chkDevice2");
var chkDeviceSTT3 = document.getElementById("chkDevice3");
var nameDevice1 = document.getElementById("nameDevice1");
var nameDevice2 = document.getElementById("nameDevice2");
var nameDevice3 = document.getElementById("nameDevice3");

// Initialize Firebase
var config = {
    apiKey: "AIzaSyD-WoIJkrA2o37-AUNrli_v-MBKBdPj7YM",
    authDomain: "luanvan2018-4a262.firebaseapp.com",
    databaseURL: "https://luanvan2018-4a262.firebaseio.com",
    projectId: "luanvan2018-4a262",
    storageBucket: "luanvan2018-4a262.appspot.com",
    messagingSenderId: "603374933595"
};
firebase.initializeApp(config);

firebase.auth().onAuthStateChanged(function(user) {
	if (user) {
    	console.log("User is signed in."); 
    
  		name = user.displayName;
  		email = user.email;
  		photoUrl = user.photoURL;
  		emailVerified = user.emailVerified;
  		uid = user.uid;  // The user's ID, unique to the Firebase project. Do NOT use
                   // this value to authenticate with your backend server, if
                   // you have one. Use User.getToken() instead.
		console.log("  Provider-specific UID: " + uid);
		console.log("  Name: " + name);
    	console.log("  Email: " + email);                 	
 		
 		var database = firebase.database().ref(uid);
 		var urlRef_Info = database.child("/info");
 		var urlRef_Status = database.child("/status"); 	

 		urlRef_Info.on("value", function(snapshot) {
			
			console.log("urlRef_Info");
			
 			console.log(JSON.stringify(snapshot)); 			
			var select_index = select.selectedIndex;
			while (select.options.length > 0) {
				select.remove(0);
			}
			// listCurrentStatus = snapshot.val();
			snapshot.forEach(function(child) {
				if(select_index == -1) //kiem tra neu la lan dau tien load page
				{
					select_index = 0;
					device_mac_key = child.key;
				}
				select.options[select.options.length] = new Option(child.val().name, child.key);
				
				
			});

			if(select_index > -1)
			{
				select.selectedIndex = select_index; //gan lai index truoc do
			}

			if(select.selectedIndex > -1)
			{
				snapshot.forEach(function(child) {
					
					if(child.key == device_mac_key)
					{
						// Lấy thông tin mô tả			
						nameDevice1.innerHTML = child.val().list['0']; //get value by key
						nameDevice2.innerHTML = child.val().list['1'];
						nameDevice3.innerHTML = child.val().list['2'];
					}
				});
			}

 		});

 		urlRef_Status.on("value", function(snapshot) {
			
			console.log("urlRef_Status");
			
 			console.log(JSON.stringify(snapshot));
 			listCurrentStatus = snapshot; 
 			snapshot.forEach(function(child) {
 				console.log(child.key); 
 				console.log(child.val());
 				if(child.key == device_mac_key)
				{
					//console.log("find ok " + device_mac_key + " : " + child.key);

					currentStatus = child.val();
																		
					if ((currentStatus & 0x01) == 0) {
						chkDeviceSTT1.checked  = false;
					}
					else
						chkDeviceSTT1.checked  = true;
					//-------------------
					if ((currentStatus & 0x02) == 0) {
						chkDeviceSTT2.checked  = false;
					}
					else
						chkDeviceSTT2.checked  = true;
					//-------------------
					if ((currentStatus & 0x04) == 0) {
						chkDeviceSTT3.checked  = false;
					}
					else
						chkDeviceSTT3.checked  = true;
				}
 			});
 		});

	} else {
    	console.log("No user is signed in.");
    	location.href = "/login";
  	}
});

select.onchange = function(){

	device_mac_key = select.value;
	console.log("selectedIndex: ", select.selectedIndex);
	console.log("device_mac_key: " + device_mac_key);

	var jsonStr = JSON.stringify(listCurrentStatus);
	console.log(jsonStr); 
	var obj = JSON.parse(jsonStr);

	currentStatus = obj[device_mac_key];

	console.log("currentStatus: " + currentStatus);
	
	if ((currentStatus & 0x01) == 0) {
		chkDeviceSTT1.checked  = false;
	}
	else
		chkDeviceSTT1.checked  = true;
	//-------------------
	if ((currentStatus & 0x02) == 0) {
		chkDeviceSTT2.checked  = false;
	}
	else
		chkDeviceSTT2.checked  = true;
	//-------------------
	if ((currentStatus & 0x04) == 0) {
		chkDeviceSTT3.checked  = false;
	}
	else
		chkDeviceSTT3.checked  = true;

	firebase.database().ref(uid + '/info/' + device_mac_key + '/list').once('value').then(function(snapshot) {		
		nameDevice1.innerHTML = snapshot.val()['0'];
		nameDevice2.innerHTML = snapshot.val()['1'];
		nameDevice3.innerHTML = snapshot.val()['2'];
  	});
};

$("#chkDevice1").click(function(){
	var estado = $(this).is(':checked');
	console.log("chkDevice1: ", estado);	
	if(select.selectedIndex > -1)
	{	
		if(estado == true)	
		{
			currentStatus |= 0x01;
		}
		else 
		{
			currentStatus &= 0xFE;
		}	
		console.log(currentStatus);

		var updates = {};
		updates["status/"+ device_mac_key] = currentStatus;
		console.log(updates);
		if(uid)
		{
			firebase.database().ref(uid).update(updates);
		}
	}
});

$("#chkDevice2").click(function(){
	var estado = $(this).is(':checked');
	console.log("chkDevice2: ", estado);

	if(select.selectedIndex > -1)
	{	
		if(estado == true)	
		{
			currentStatus |= 0x02;
		}
		else 
		{
			currentStatus &= 0xFD;
		}	
		console.log(currentStatus);

		var updates = {};
		updates["status/"+ device_mac_key] = currentStatus;
		console.log(updates);
		if(uid)
		{
			firebase.database().ref(uid).update(updates);
		}
	}
});

$("#chkDevice3").click(function(){
	var estado = $(this).is(':checked');
	console.log("chkDevice3: ", estado);

	if(select.selectedIndex > -1)
	{	
		if(estado == true)	
		{
			currentStatus |= 0x04;
		}
		else 
		{
			currentStatus &= 0xFB;
		}	
		console.log(currentStatus);

		var updates = {};
		updates["status/"+ device_mac_key] = currentStatus;
		console.log(updates);
		if(uid)
		{
			firebase.database().ref(uid).update(updates);
		}
	}
});

$("#logout").click(function(){
	
	console.log("logout");	
	// [START signout]
	firebase.auth().signOut();
	// [END signout]
	location.href = "/";
});
