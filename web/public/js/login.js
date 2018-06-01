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
		window.location.href = "/home"
    }
    else
    {
    	console.log("No user is signed in.");
    }
});

function login() {
	var email = document.getElementById('username').value;
	var password = document.getElementById('password').value;
	if (email.length < 4) {
		alert('Vui lòng nhập địa chỉ email.');
		return;
	}
	if (password.length < 4) {
		alert('Vui lòng nhập mật khẩu.');
		return;
	}
	// Sign in with email and pass.
	// [START authwithemail]
	firebase.auth().signInWithEmailAndPassword(email, password).catch(function(error) {
		// Handle Errors here.
		var errorCode = error.code;
		var errorMessage = error.message;
		// [START_EXCLUDE]
		if (errorCode === 'auth/wrong-password') {
			alert('Nhập sai mật khẩu.');
		} else {
			alert(errorMessage);
		}
		console.log(error);		
		// [END_EXCLUDE]
	});
	// [END authwithemail]
}

document.getElementById('login-button').addEventListener('click', login, false);