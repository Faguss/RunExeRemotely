<?php
// Script for RunExeRemotely by Faguss (ofp-faguss.com)

define("DB_FILE", "ofpserverremote.json");
define("ADMIN_IP", ['127.0.0.1']);

if (!file_exists(DB_FILE)) {
	$db = ["current"=>["keyname"=>"","argline"=>""], "options"=>["Turn off"=>"?turnoff"]];
	file_put_contents(DB_FILE, json_encode($db));
} else {
	$db = json_decode(file_get_contents(DB_FILE), true);
	if (!isset($db)) 
		die("Failed to read database");
}

// Return just the value
if (isset($_GET['arg'])) {
	echo $db["current"]["argline"];
	exit();
}

// Only allow admin to modify database
if (!in_array($_SERVER['REMOTE_ADDR'], ADMIN_IP)) {
	http_response_code(403);
	die('Forbidden');
}

// Add new value to the database
if (isset($_POST['new_db_key']) && isset($_POST['new_db_value'])) {
	$db["options"][$_POST['new_db_key']] = $_POST['new_db_value'];
	file_put_contents(DB_FILE, json_encode($db));
}

// Remove value from the database
if (isset($_POST['delete_db_key']) && isset($db["options"][$_POST['delete_db_key']])) {
	unset($db["options"][$_POST['delete_db_key']]);
	file_put_contents(DB_FILE, json_encode($db));
}

// Change currently selected value in the database
if (isset($_POST['new_db_current']) && isset($db["options"][$_POST['new_db_current']])) {
	$db["current"]["keyname"] = $_POST['new_db_current'];
	$db["current"]["argline"] = $db["options"][$_POST['new_db_current']];
	file_put_contents(DB_FILE, json_encode($db));
}
?>

<!DOCTYPE html>
<html lang="en">
<head>
	<meta charset="UTF-8">
	<title>OFP Server Remote Control</title>
	<meta name="viewport" content="width=device-width,initial-scale=1">
</head>
<body>
	<h2>List of options:</h2>
	<table border="1" cellpadding="5" cellspacing="5">
	<?php
		foreach($db["options"] as $key=>$argline) {
			$current = $key == $db["current"]["keyname"];

			if ($current)
				echo "<tr bgcolor=\"yellow\">";
			else
				echo "<tr>";

			echo "
			<td>";

			if (!$current)
				echo "
					<form method=\"post\" action=\"{$_SERVER['PHP_SELF']}\">
						<input type=\"hidden\" name=\"new_db_current\" value=\"$key\" />
						<button type=\"submit\">$key</button>
					</form>";
			else
				echo $key;

			echo "
			</td>
			<td><small>$argline</small></td>
			<td>
				<form method=\"post\" action=\"{$_SERVER['PHP_SELF']}\">
					<input type=\"hidden\" name=\"delete_db_key\" value=\"$key\" />
					<button type=\"submit\" onclick=\"return confirm('Are you sure?')\"><small>Remove</small></button>
				</form>
			</td>

			</tr>";
		}
	?>
	</table>

	<br><hr><br>

	<h2>Add new option:</h2>
	<form id="form1" method="post" action="<?=$_SERVER['PHP_SELF'];?>">
		<label for="new_db_key">Name:</label>
		<input type="text" id="new_db_key" name="new_db_key">
		<label for="new_db_value">Argument line:</label>
		<input type="text" id="new_db_value" name="new_db_value">
		<button type="submit" form="form1" value="Submit">Submit</button>
	</form>
</body>
</html>