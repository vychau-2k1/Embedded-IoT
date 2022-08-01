<?php 
    //Connect to database
    $server = "localhost"; //IP of server
    $user = "admin";
    $pass = "password";
    $database = "snakegame";

    $conn = mysqli_connect($server, $user, $pass, $database);
    //check connection
    if(mysqli_connect_errno())
    {
        echo "Fail to connect database: " . mysqli_connect_error();
        exit();
    }
?>