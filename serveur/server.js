var express = require('express');
var server = express();
var path = require('path');

server.use(express.static(__dirname + '/public'));

var mysql = require('mysql')
var pool = mysql.createPool({
    host: 'localhost',
    user: 'iot_project',
    password: 'iot_epitech',
    database: 'iot_project'
})

/*server.get('/', function(req, res) {
    res.sendFile(path.join(__dirname + "/index.html"));
});*/

server.get('/validations', function(req, res) {
    pool.getConnection(function(err, connection) {
        console.log("yes");
        if (err) throw err;
        pool.query("SELECT * FROM validation", function (err, results, fields) {
            if (err) throw err;
            console.log(results);
            res.send(JSON.stringify(results));
            connection.release();
        });
    });    
});


server.listen(3000);