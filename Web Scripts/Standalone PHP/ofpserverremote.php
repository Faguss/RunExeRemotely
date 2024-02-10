<?php
// Script for RunExeRemotely by Faguss (ofp-faguss.com)

if (empty($_SERVER["HTTPS"]) || $_SERVER["HTTPS"] !== "on")
	die("Requires HTTPS");

if (version_compare(PHP_VERSION, '7.1.0', '<'))
	die("Requires PHP 7.1");

// Recommended to keep files in a non-public folder
define("PATH_TO_DB_FILE", "ofpserverremote.json");
define("PATH_TO_PASSWORD_FILE",'ofpserverremote.pass');
define("PAGE_TITLE", "OFP Server Remote Control");

session_start([
    'cookie_lifetime' => 60 * 15,
	'cookie_secure'   => true,
	'cookie_httponly' => true,
	'cookie_samesite' => 'Lax',
]);

// Load database or generate it
if (file_exists(PATH_TO_DB_FILE)) {
	$db = json_decode(file_get_contents(PATH_TO_DB_FILE), true);
	if (!isset($db)) 
		die("Failed to read database");
} else {
	$db = [
		"current" => [
			"keyname" => "",
			"argline" => "",
		], 
		"options" => [
			"Turn off" => "?turnoff",
			"Shutdown" => "?shutdown",
			"FDF"      => "-nomap -config=server.cfg -mod=finmod",
		]
	];
	file_put_contents(PATH_TO_DB_FILE, json_encode($db));
}

// Just return the value
if (isset($_GET['arg'])) {
	header("Content-Type: text/plain");
	header("Cache-Control: no-store, no-cache, must-revalidate, max-age=0");
	header("Cache-Control: post-check=0, pre-check=0", false);
	header("Pragma: no-cache");
	echo $db["current"]["argline"];
	exit();
}

// Database actions
if (!empty($_POST['token']) && isset($_SESSION['token']) && hash_equals($_POST['token'],$_SESSION['token'])) {
	if (isset($_SESSION['logged'])) {
		// Add new
		if (isset($_POST['new_db_key']) && isset($_POST['new_db_value'])) {
			$db["options"][$_POST['new_db_key']] = $_POST['new_db_value'];
			file_put_contents(PATH_TO_DB_FILE, json_encode($db));
			header("Location: {$_SERVER['PHP_SELF']}");
		}

		// Remove
		$remove_key = isset($_POST['delete_db_key']) ? html_entity_decode($_POST['delete_db_key'], ENT_QUOTES) : "";
		
		if (!empty($remove_key) && isset($db["options"][$remove_key])) {
			unset($db["options"][$remove_key]);
			file_put_contents(PATH_TO_DB_FILE, json_encode($db));
			header("Location: {$_SERVER['PHP_SELF']}");
		}

		// Change current selection
		$current_key = isset($_POST['new_db_current']) ? html_entity_decode($_POST['new_db_current'], ENT_QUOTES) : "";
		
		if (!empty($current_key) && isset($db["options"][$current_key])) {
			$db["current"]["keyname"] = $current_key;
			$db["current"]["argline"] = $db["options"][$current_key];
			file_put_contents(PATH_TO_DB_FILE, json_encode($db));
			header("Location: {$_SERVER['PHP_SELF']}");
		}
		
		// Logout
		if (isset($_POST['logout'])) {
			header("Location: {$_SERVER['PHP_SELF']}");
			session_unset();
			session_destroy();
		}
	} else {
		// Login / set password
		$password_sent = filter_input(INPUT_POST, 'password', FILTER_SANITIZE_STRING);
				
		if (!empty($password_sent)) {
			if (file_exists(PATH_TO_PASSWORD_FILE)) {
				$password_saved = file_get_contents(PATH_TO_PASSWORD_FILE);
				
				if (password_verify($password_sent,$password_saved)) {
					session_regenerate_id(true);
					$_SESSION['logged'] = true;
				}
			} else {
				file_put_contents(PATH_TO_PASSWORD_FILE, password_hash($password_sent, PASSWORD_DEFAULT));
			}
		}
	}
}

$_SESSION['token'] = bin2hex(random_bytes(32));
?>

<!DOCTYPE html>
<html lang="en">
<head>
	<meta charset="UTF-8">
	<title><?=PAGE_TITLE?></title>
	<meta name="viewport" content="width=device-width,initial-scale=1">
	<style>
	body {
		background: rgb(233,233,233);
		background: linear-gradient(90deg, rgba(233,233,233,1) 0%, rgba(237,225,225,1) 24%, rgba(225,242,246,1) 100%);
	}
	h1 {
		color: #f7ff00;
		text-shadow: rgb(0, 0, 0) 3px 0px 0px, rgb(0, 0, 0) 2.83487px 0.981584px 0px, rgb(0, 0, 0) 2.35766px 1.85511px 0px, rgb(0, 0, 0) 1.62091px 2.52441px 0px, rgb(0, 0, 0) 0.705713px 2.91581px 0px, rgb(0, 0, 0) -0.287171px 2.98622px 0px, rgb(0, 0, 0) -1.24844px 2.72789px 0px, rgb(0, 0, 0) -2.07227px 2.16926px 0px, rgb(0, 0, 0) -2.66798px 1.37182px 0px, rgb(0, 0, 0) -2.96998px 0.42336px 0px, rgb(0, 0, 0) -2.94502px -0.571704px 0px, rgb(0, 0, 0) -2.59586px -1.50383px 0px, rgb(0, 0, 0) -1.96093px -2.27041px 0px, rgb(0, 0, 0) -1.11013px -2.78704px 0px, rgb(0, 0, 0) -0.137119px -2.99686px 0px, rgb(0, 0, 0) 0.850987px -2.87677px 0px, rgb(0, 0, 0) 1.74541px -2.43999px 0px, rgb(0, 0, 0) 2.44769px -1.73459px 0px, rgb(0, 0, 0) 2.88051px -0.838247px 0px;
	}
	</style>
</head>
<body>
<h1><?=PAGE_TITLE?></h1>
<hr><br>

<?php
if (isset($_SESSION['logged'])) {
	?>
	<h2>List of options:</h2>
	<table border="1" cellpadding="5" cellspacing="5" style="background-color:#ffffff;">
	<?php
		foreach($db["options"] as $key=>$argline) {
			$key_formatted = htmlentities($key, ENT_QUOTES);
			
			if ($key == $db["current"]["keyname"]) :
				?> 
				<tr bgcolor="yellow"> 
				<td>
				<?php
				echo $key_formatted;
			else :
				?> 
				<tr>
				<td>
				<form method="post" action="<?=$_SERVER['PHP_SELF']?>">
					<input type="hidden" name="token" value="<?=$_SESSION['token']?>">
					<input type="hidden" name="new_db_current" value="<?=$key_formatted?>" />
					<button type="submit"><?=$key_formatted?></button>
				</form>
				<?php
			endif; 

			?>
			</td>
			<td><small><?=htmlentities($argline, ENT_QUOTES)?></small></td>
			<td>
				<form method="post" action="<?=$_SERVER['PHP_SELF']?>">
					<input type="hidden" name="token" value="<?=$_SESSION['token']?>">
					<input type="hidden" name="delete_db_key" value="<?=$key_formatted?>" />
					<button type="submit" onclick="return confirm('Are you sure?')"><small>Remove</small></button>
				</form>
			</td>

			</tr><?php
		}
	?>
	</table>

	<br><br><hr><br>

	<h2>Add new option:</h2>
	<form id="form1" method="post" action="<?=$_SERVER['PHP_SELF']?>">
		<input type="hidden" name="token" value="<?=$_SESSION['token']?>">
		<label for="new_db_key">Name:</label>
		<input type="text" id="new_db_key" name="new_db_key">
		<label for="new_db_value">Argument line:</label>
		<input type="text" id="new_db_value" name="new_db_value">
		<button type="submit" form="form1" value="Submit">Submit</button>
	</form>
	
	<br><br><hr><br>
	
	<h2>Address to the arguments:</h2>
	<?=(empty($_SERVER['HTTPS']) ? 'http' : 'https') . "://$_SERVER[HTTP_HOST]$_SERVER[REQUEST_URI]?arg"?>
	
	<br><br><br><hr><br><br>
	
	<form id="form2" method="post" action="<?=$_SERVER['PHP_SELF']?>">
		<input type="hidden" name="token" value="<?=$_SESSION['token']?>">
		<button type="submit" form="form2" name="logout">Logout</button>
	</form>
	<?php
} else {
	?>
	<form method="post" action="<?=$_SERVER['PHP_SELF']?>">
		<input type="hidden" name="token" value="<?=$_SESSION['token']?>">
		<input type="password" name="password">
		<input type="submit" value="<?=file_exists(PATH_TO_PASSWORD_FILE) ? "Enter Password" : "Set New Password" ?> ">
	</form>
	<?php
}
?>
</body>
</html>