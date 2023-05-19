<!DOCTYPE html>
<html lang="en">
<head>
	<meta charset="UTF-8">
	<title>OFP Server Remote Control</title>
	<meta name="viewport" content="width=device-width,initial-scale=1">
</head>

<?php
define("EXE_ARGUMENTS", [
	"bas"    => '-mod=@bas', 
	"ww4"    => '-mod=@ww4mod25',
	"finmod" => '-mod=finmod'
]);

$all_options   = array_merge(["Turn off"=>"?turnoff"], EXE_ARGUMENTS);
$save_file     = substr(str_replace(".php", ".txt", $_SERVER['PHP_SELF']), 1);
$save_contents = "";

if (isset($_POST['batscript']) && isset($all_options[$_POST['batscript']])) {
	$save_contents = $all_options[$_POST['batscript']];
	file_put_contents($save_file, $save_contents);
} else {
	$save_contents = file_get_contents($save_file);
	if (!in_array($save_contents, $all_options))
		$save_contents = $all_options[0];
}
?>

<body>
	<form method="post" action="<?=$_SERVER['PHP_SELF'];?>">
		<label for="batscript">Current option:</label>
		<select id="batscript" name="batscript" onChange="this.form.submit()">
		<?php
			foreach($all_options as $display=>$value)
				echo "<option value=\"$display\"" . ($value==$save_contents ? " selected=\"selected\"" : "") . ">$display</option>";
		?>
		</select>
	</form>
</body>

</html>