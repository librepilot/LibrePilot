<?php

if (!$_SERVER['REQUEST_METHOD'] === 'GET' || empty($_GET['hash']))  {
    usage_error();
}

// NOTE: $hash not sanitized because of the checksum match test.
$hash = $_GET['hash'];
$string = str_replace('&hash=' . $hash, '', rawurldecode($_SERVER['QUERY_STRING']));

if ($hash != md5($string)) {
    usage_error();
}


$dbhost   = 'localhost';
$dbname   = '';
$dbpasswd = '';
$dbuser   = '';

$db = new mysqli($dbhost, $dbuser, $dbpasswd, $dbname);
unset($dbhost, $dbuser, $dbpasswd, $dbname);

if ($db->connect_error) {
    die();
}

$sql = sprintf("SELECT id, last_date FROM usagetracker WHERE hash = '%s' LIMIT 1", 
    $db->real_escape_string($hash)
);

$res = $db->query($sql);
if ($res->num_rows > 0) {
    // Shouldn't normally be here but happens if GCS settings are reset
    // or if the request come from another source than GCS.

    $hashUpdate = $res->fetch_assoc();
    if ($hashUpdate['last_date'] < (time() - 3600)) {
        // Update timestamp and connection count.
        $sql = sprintf("UPDATE usagetracker SET last_date = %u, count = count + 1 WHERE id = %u LIMIT 1",
            time(),
            $hashUpdate['id']
        );
        $db->query($sql);
    }
}
else {
    // New hash
    $sql = sprintf("INSERT INTO usagetracker (first_date, last_date, ip, data, hash)
                         VALUES (%u, %u, '%s', '%s', '%s')",
        time(),
        time(),
        encode_ip($_SERVER['REMOTE_ADDR']),
        $db->real_escape_string($string),
        $db->real_escape_string($_GET['hash'])
    );

    $db->query($sql);
}

$db->close();


function usage_error()
{
    //ob_start();

    header($_SERVER["SERVER_PROTOCOL"]." 404 Not Found");
    header("Status: 404 Not Found");

    // Matching server 404 output can be added.
    exit();
}

function encode_ip($dotquad_ip)
{
    $ip_sep = explode('.', $dotquad_ip);
    return sprintf('%02x%02x%02x%02x', $ip_sep[0], $ip_sep[1], $ip_sep[2], $ip_sep[3]);
}

?>
