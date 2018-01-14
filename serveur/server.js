var express = require('express');
var server = express.createServer();
server.listen(3000);

var mysql = require('mysql')
var pool = mysql.createPool({
    host: 'localhost',
    user: 'benjamin',
    password: 'azerty',
    database: 'iot2_project'
})

server.get('/', function(request, response) {
    pool.getConnection(function(err, connection) {
        if (err) throw err;
        pool.query("SELECT * FROM rfid_key", function (err, results, fields) {
            if (err) throw err;
            console.log(results);
            response.send(JSON.stringify(results));
            connection.release();
        });
    });
});


// MySql Connection



