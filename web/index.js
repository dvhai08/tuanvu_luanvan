const express = require('express');
var app = express();

const bodyParser = require('body-parser');
app.use(bodyParser.urlencoded({ extended: false })); // parse application/x-www-form-urlencoded
app.use(bodyParser.json()); // parse application/json

app.use(express.static("public"));
app.set("view engine","ejs");
app.set("views","./views");


app.get('/', function (req, res) {  	
  	res.render("login");  		
});

app.get('/login', function (req, res) {  	
  	res.render("login");  		
});

app.get('/home', function (req, res) {  	
  	res.render("home");  		
});

app.get('/logout', function (req, res) {	
  	res.render("login");  		
});

var port = process.env.PORT || 8000;

app.listen(port, function(){
	console.log('listening on port ' + port);
});