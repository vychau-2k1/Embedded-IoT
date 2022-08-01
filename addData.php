<?php
include("config.php");
// gui data xuong database
$sql = "update control set start = 0 where id = 1";
if(!mysqli_query($conn,$sql))
{
    echo "error:" . mysqli_error($conn);
}
//mysqli_query($conn, $sql);


// ngat ket noi voi database
mysqli_close($conn);
?>
