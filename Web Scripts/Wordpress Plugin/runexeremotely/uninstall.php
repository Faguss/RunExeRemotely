<?php
if (!defined('WP_UNINSTALL_PLUGIN')) die;
delete_option("runexeremotely_table_version");
global $wpdb;
$wpdb->query("DROP TABLE IF EXISTS {$wpdb->prefix}rer_executables");
$wpdb->query("DROP TABLE IF EXISTS {$wpdb->prefix}rer_options");