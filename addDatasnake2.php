<?php
include("config.php");
// gui data xuong database
$winner = $_POST["winner2"];
$score = $_POST["score_2"];
$sql = "update control set score = $score, winner = $winner where id = 2";
if(!mysqli_query($conn,$sql))
{
    echo "error:" . mysqli_error($conn);
}
//mysqli_query($conn, $sql);


// ngat ket noi voi database
mysqli_close($conn);
?>
