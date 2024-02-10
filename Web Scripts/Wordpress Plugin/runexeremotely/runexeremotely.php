<?php
/*
Plugin Name: Run Exe Remotely
Description: Control panel for the RunExeRemotely program
Plugin URI: https://github.com/Faguss/RunExeRemotely
Author URI: https://ofp-faguss.com
Author: Faguss
License: Public Domain
Version: 1.0
*/

if (!class_exists('RunExeRemotely_Table')) {
	if (!class_exists('WP_List_Table')) {
		require_once(ABSPATH . 'wp-admin/includes/class-wp-list-table.php');
	}

	class RunExeRemotely_Table extends WP_List_Table {
		public $table_name     = "";
		public $page_name      = "";
		public $columns        = [];
		public $delete_message = ["count"=>0, "message"=>""];
		
		function __construct($input) {
			global $wpdb;
			$this->table_name = $wpdb->prefix . $input["table_name"];
			$this->page_name  = "runexeremotely_" . $input["page_name"] . "_edit";
			$this->columns    = $input["columns"];

			parent::__construct([
				'singular' => $input["singular"],
				'plural'   => $input["plural"],
			]);
		}

		function column_default($item, $column_name) {
			return stripslashes(esc_html($item[$column_name]));
		}

		function column_name($item) {
			global $wpdb;
			$actions = [
				'link'   => "<a href=\"".get_site_url()."/wp-json/runexeremotely/exe/".urlencode($item['name'])."\" target=\"_blank\">Link to data</a>",
				'edit'   => "<a href=\"?page={$this->page_name}&id={$item['id']}\">Edit</a>",
				'delete' => "<a href=\"?page={$_REQUEST['page']}&action=delete&id={$item['id']}\" onclick=\"return confirm('Are you sure?')\">Delete</a>",
			];
			
			if ($this->table_name == ($wpdb->prefix."rer_options"))
				unset($actions['link']);
				
			return esc_html($item['name']) . " " . $this->row_actions($actions);
		}

		function column_cb($item) {
			return "<input type='checkbox' name='id[]' value='{$item['id']}'>";
		}

		function get_columns() {
			return array_merge(['cb'=>'<input type="checkbox" />'],$this->columns);
		}

		function get_sortable_columns() {
			$sortable_columns = [];
			
			foreach($this->columns as $key=>$value)
				$sortable_columns[] = [$key, true];

			return $sortable_columns;
		}

		function get_bulk_actions() {
			return [
				'delete' => 'Delete'
			];
		}

		function process_bulk_action() {
			global $wpdb;
			$wpdb->show_errors();
			
			if ($this->current_action() === 'delete') {
				$options     = $wpdb->prefix."rer_options";
				$executables = $wpdb->prefix."rer_executables";
				
				$ids = isset($_REQUEST['id']) ? $_REQUEST['id'] : [];
				if (!is_array($ids)) 
					$ids = [$ids];
				
				if ($this->table_name == $options) {
					$item = $wpdb->get_results(
						$wpdb->prepare("
							SELECT 
								$options.name, 
								$options.id 
							FROM 
								$options JOIN $executables ON $options.id=$executables.option_id 
							WHERE 
								$options.id IN(".implode(', ', array_fill(0, count($ids), '%d')).")", $ids
						), ARRAY_A
					);
					
					$busy = [];
					foreach($item as $row) {
						$busy[] = esc_html($row["name"]);
						$ids    = array_diff($ids, [$row["id"]]);
					}

					if (!empty($busy))
						$this->delete_message["message"] = "Can't delete " . implode(', ', $busy) . " because they are used";
				}
				
				if (!empty($ids)) {
					$this->delete_message["count"] = $wpdb->query(
						$wpdb->prepare("
							DELETE FROM 
								{$this->table_name} 
							WHERE 
								id IN(".implode(', ', array_fill(0, count($ids), '%d')).")", $ids
						)
					);
				}
			}
			
			return 0;
		}

		function prepare_items() {
			global $wpdb;
			$per_page = 5;
			$columns  = $this->get_columns();
			$hidden   = [];
			$sortable = $this->get_sortable_columns();

			$this->_column_headers = [$columns, $hidden, $sortable];

			$this->process_bulk_action();

			$total_items = $wpdb->get_var("SELECT COUNT(id) FROM {$this->table_name}");

			$paged   = isset($_REQUEST['paged']) ? ($per_page * max(0, intval($_REQUEST['paged']) - 1)) : 0;
			$orderby = (isset($_REQUEST['orderby']) && in_array($_REQUEST['orderby'], array_keys($this->get_sortable_columns()))) ? $_REQUEST['orderby'] : 'name';
			$order   = (isset($_REQUEST['order']) && in_array($_REQUEST['order'], ['asc', 'desc'])) ? $_REQUEST['order'] : 'asc';

			$executables = ($wpdb->prefix."rer_executables");
			$options     = ($wpdb->prefix."rer_options");
					
			if ($this->table_name == $executables)
				$this->items = $wpdb->get_results(
					$wpdb->prepare(
						"SELECT 
							$executables.*, 
							$options.name AS option_name 
						FROM 
							$executables JOIN $options ON $executables.option_id=$options.id 
						ORDER BY 
							$orderby $order 
						LIMIT 
							%d 
						OFFSET 
							%d", $per_page, $paged
					), ARRAY_A
				);
			else
				$this->items = $wpdb->get_results(
					$wpdb->prepare(
						"SELECT 
							* 
						FROM 
							{$this->table_name} 
						ORDER BY 
							$orderby $order 
						LIMIT 
							%d 
						OFFSET 
							%d", $per_page, $paged
					), ARRAY_A
				);
				
			$this->set_pagination_args([
				'total_items' => $total_items,
				'per_page'    => $per_page,
				'total_pages' => ceil($total_items / $per_page)
			]);
		}
	}
}

if (!class_exists('RunExeRemotely')) {
	class RunExeRemotely {
		static $instance             = false;
		public $table_version        = "1.0";
		public $table_version_option = "runexeremotely_table_version";
		public $table_options        = "rer_options";
		public $table_executables    = "rer_executables";
		
		public static function getInstance() {
			if (!self::$instance)
				self::$instance = new self;
			return self::$instance;
		}
		
		private function __construct() {
			global $wpdb;
			$this->table_options     = $wpdb->prefix . $this->table_options;
			$this->table_executables = $wpdb->prefix . $this->table_executables;
			
			register_activation_hook(__FILE__, [&$this, 'create_table']);
			register_uninstall_hook(__FILE__, [&$this, 'delete_table']);
			
			add_action('admin_menu', [&$this, 'populate_admin_menu']);
			add_action('rest_api_init', function () {
				register_rest_route('runexeremotely', '/exe/(?P<title>[\S]+)', [
					'methods'  => 'GET',
					'callback' => [&$this, 'get_arg_line'],
					'permission_callback' => '__return_true',
				]);
			});
		}
		
		public function create_table() {
			global $wpdb;
			$sql = "
				CREATE TABLE {$this->table_options} (
					id INT NOT NULL AUTO_INCREMENT,
					name VARCHAR(100) NOT NULL,
					argline TEXT NOT NULL,
					PRIMARY KEY (id)
				);
				
				CREATE TABLE {$this->table_executables} (
					id int(11) NOT NULL AUTO_INCREMENT,
					name VARCHAR(100) NOT NULL,
					option_id INT,
					PRIMARY KEY (id),
					FOREIGN KEY (option_id) REFERENCES {$this->table_options}(id)
				);
			";

			require_once(ABSPATH . 'wp-admin/includes/upgrade.php');
			dbDelta($sql);

			add_option($this->table_version_option, $this->table_version);
			
			$turnoff = $wpdb->get_var("SELECT id FROM {$this->table_options} WHERE argline='?turnoff'");
			
			if (empty($turnoff)) {
				$wpdb->insert($this->table_options, [
					'name'    => 'Turn off',
					'argline' => '?turnoff'
				]);
				
				$wpdb->insert($this->table_options, [
					'name'    => 'Shutdown computer',
					'argline' => '?shutdown'
				]);
			}
		}
		
		public function delete_table() {
			global $wpdb;
			$wpdb->query("DROP TABLE IF EXISTS {$this->table_options}");
			$wpdb->query("DROP TABLE IF EXISTS {$this->table_executables}");
			delete_option($this->table_version_option);
		}
		
		public function populate_admin_menu() {
			add_menu_page(
				'Main', //page title
				'Run Exe Remotely', //menu title
				'activate_plugins', //capability
				'runexeremotely_options', //menu_slug
				[&$this, 'display_options_table'] //callback
			);
			add_submenu_page(
				'runexeremotely_options', //parent_slug
				'Display options', //page title
				'Options', //menu title
				'activate_plugins', //capability
				'runexeremotely_options', //menu_slug
				[&$this, 'display_options_table'] //callback
			);
			add_submenu_page(
				'runexeremotely_options', //parent_slug
				'Edit options', //page title
				'Options - Edit', //menu title
				'activate_plugins', //capability
				'runexeremotely_options_edit', //menu_slug
				[&$this, 'display_options_edit'] //callback
			);
			add_submenu_page(
				'runexeremotely_options', //parent_slug
				'Display executables', //page title
				'Executables', //menu title
				'activate_plugins', //capability
				'runexeremotely_executables', //menu_slug
				[&$this, 'display_executables_table'] //callback
			);
			add_submenu_page(
				'runexeremotely_options', //parent_slug
				'Edit executables', //page title
				'Executables - Edit', //menu title
				'activate_plugins', //capability
				'runexeremotely_executables_edit', //menu_slug
				[&$this, 'display_executables_edit'] //callback
			);
		}
		
		public function get_arg_line($executable) {
			global $wpdb;
			$item = $wpdb->get_var(
				$wpdb->prepare(
					"SELECT 
						{$this->table_options}.argline 
					FROM 
						{$this->table_executables} JOIN {$this->table_options} ON {$this->table_executables}.option_id={$this->table_options}.id 
					WHERE 
						{$this->table_executables}.name=%s", urldecode($executable["title"])
				)
			);

			if (empty($item))
				return new WP_REST_Response(['error'=>'Item not found'], 404);

			header("Content-Type: text/plain");
			header("Cache-Control: no-store, no-cache, must-revalidate, max-age=0");
			header("Cache-Control: post-check=0, pre-check=0", false);
			header("Pragma: no-cache");
			echo stripslashes($item);
			exit();
		}

		public function display_options_table() {
			global $wpdb;		
			$table = new RunExeRemotely_Table([
				"table_name" => "rer_options", 
				"singular"   => "rer_option", 
				"plural"     => "rer_options", 
				"page_name"  => "options", 
				"columns"    => ["name"=>"Title","argline"=>"Argument Line"]
			]);
			$table->prepare_items();

			?>
			<div class="wrap">
				<div class="icon32 icon32-posts-post" id="icon-edit"><br></div>
				<h2>Options <a class="add-new-h2" href="<?= get_admin_url(get_current_blog_id(), 'admin.php?page=runexeremotely_options_edit');?>">Add new</a></h2>
				<?php 
				if ($table->current_action() === "delete") {
					if (!empty($table->delete_message["message"]))
						echo '<div class="updated below-h2" id="message"><p>'.$table->delete_message["message"].'</p></div>';
					
					echo '<div class="updated below-h2" id="message"><p>Items deleted: '.$table->delete_message["count"].'</p></div>';
				}
				?>

				<form id="options-table" method="GET">
					<input type="hidden" name="page" value="<?= $_REQUEST['page'] ?>"/>
					<?php $table->display() ?>
				</form>

			</div>
			<?php
		}
		
		public function display_options_edit() {
			global $wpdb;
			$message = "";
			$notice  = "";
			$default = [
				"id"      => 0,
				"name"    => "",
				"argline" => ""
			];

			if (isset($_REQUEST['nonce']) && wp_verify_nonce($_REQUEST['nonce'], basename(__FILE__))) {
				$item            = shortcode_atts($default, $_REQUEST);
				$item_valid      = $this->display_options_edit_validate($item);
				$item["name"]    = sanitize_text_field($item["name"]);
				$item["argline"] = sanitize_text_field($item["argline"]);
				
				if ($item_valid === true) {
					if ($item["id"] == 0) {
						$result     = $wpdb->insert($this->table_options, $item);
						$item["id"] = $wpdb->insert_id;
						
						if ($result) {
							$message = "Item was successfully saved";
						} else {
							$notice = "There was an error while saving item";
						}
					} else {
						$result = $wpdb->update($this->table_options, $item, ["id"=>$item["id"]]);
						
						if ($result) {
							$message = "Item was successfully updated";
						} else {
							$notice = "There was an error while updating item";
						}
					}
				} else {
					$notice = $item_valid;
				}
			} else {
				$item = $default;
				
				if (isset($_REQUEST["id"])) {
					$item = $wpdb->get_row(
						$wpdb->prepare(
							"SELECT 
								* 
							FROM 
								{$this->table_options} 
							WHERE 
								id = %d", $_REQUEST['id']
						), ARRAY_A
					);
					
					if (!$item) {
						$item   = $default;
						$notice = "Item not found";
					}
				}
			}

			add_meta_box(
				'options_form_meta_box', //id
				'Fill options details', //title
				[&$this,'options_add_new_metabox'], //callback
				"", //screen
				'normal', //context
				'default' //priority
			);

			?>
			<div class="wrap">
				<div class="icon32 icon32-posts-post" id="icon-edit"><br></div>
				<h2><?=$item['id']==0?"Add New Option":"Edit Option"?> 
				<a class="add-new-h2" href="<?=get_admin_url(get_current_blog_id(), 'admin.php?page=runexeremotely_options');?>">Back to list</a>
				<?php if ($item['id']!=0) : ?>
				<a class="add-new-h2" href="<?=get_admin_url(get_current_blog_id(), 'admin.php?page=runexeremotely_options_edit');?>">Add New</a>
				<?php endif; ?>
				</h2>

				<?php if (!empty($notice)): ?>
				<div id="notice" class="error"><p><?=$notice ?></p></div>
				<?php endif;?>
				
				<?php if (!empty($message)): ?>
				<div id="message" class="updated"><p><?=$message ?></p></div>
				<?php endif;?>

				<form id="form" method="POST">
					<input type="hidden" name="nonce" value="<?=wp_create_nonce(basename(__FILE__))?>"/>
					<input type="hidden" name="id" value="<?=$item['id'] ?>"/>

					<div class="metabox-holder" id="poststuff">
						<div id="post-body">
							<div id="post-body-content">
								<?php do_meta_boxes('', 'normal', $item); ?>
								<input type="submit" value="Save" id="submit" class="button-primary" name="submit">
							</div>
						</div>
					</div>
				</form>
			</div>
			<?php
		}
		
		function display_options_edit_validate($item) {
			$messages = [];
			
			if (!ctype_digit($item['id']))
				$messages[] = 'Invalid id';

			if (empty($item['name'])) 
				$messages[] = 'Name is required';
			
			if (empty($item['argline'])) 
				$messages[] = 'Argument line is required';

			if (empty($messages)) 
				return true;
			else
				return implode("<br />", $messages);
		}
		
		function options_add_new_metabox($item) {
			?>
			<table cellspacing="2" cellpadding="5" style="width: 100%;" class="form-table">
				<tbody>
				<tr class="form-field">
					<th valign="top" scope="row">
						<label for="name">Title</label>
					</th>
					<td>
						<input id="name" name="name" type="text" style="width: 95%" value="<?php echo esc_attr($item['name'])?>"
								size="50" class="code" placeholder="cfog" required>
					</td>
				</tr>
				<tr class="form-field">
					<th valign="top" scope="row">
						<label for="argline">Argument line</label>
					</th>
					<td>
						<input id="argline" name="argline" type="text" style="width: 95%" value="<?=stripslashes(esc_attr($item['argline']))?>" size="50" class="code" placeholder="-nosplash -nomap -mod=cfog" required>
					</td>
				</tr>
				</tbody>
			</table>
			<?php
		}
		
		public function display_executables_table() {
			global $wpdb;
			$table = new RunExeRemotely_Table([
				"table_name" => "rer_executables", 
				"singular"   => "rer_executable", 
				"plural"     => "rer_executables", 
				"page_name"  => "executables", 
				"columns"    => ["name"=>"Name","option_name"=>"Selected Option"]
			]);
			$table->prepare_items();

			?>
			<div class="wrap">
				<div class="icon32 icon32-posts-post" id="icon-edit"><br></div>
				<h2>Executables <a class="add-new-h2" href="<?= get_admin_url(get_current_blog_id(), 'admin.php?page=runexeremotely_executables_edit');?>">Add new</a></h2>
				<?php
				if ($table->current_action() === "delete") {					
					echo '<div class="updated below-h2" id="message"><p>Items deleted: '.$table->delete_message["count"].'</p></div>';
				}
				?>

				<form id="executables-table" method="GET">
					<input type="hidden" name="page" value="<?= $_REQUEST['page'] ?>"/>
					<?php $table->display() ?>
				</form>

			</div>
			<?php
		}
		
		public function display_executables_edit() {
			global $wpdb;
			$message = "";
			$notice  = "";
			$default = [
				"id"        => 0,
				"name"      => "",
				"option_id" => 0
			];

			if (isset($_REQUEST['nonce']) && wp_verify_nonce($_REQUEST['nonce'], basename(__FILE__))) {
				$item         = shortcode_atts($default, $_REQUEST);				
				$item_valid   = $this->display_executables_edit_validate($item);
				$item["name"] = sanitize_text_field($item["name"]);
				
				if ($item_valid === true) {
					if ($item["id"] == 0) {
						$result     = $wpdb->insert($this->table_executables, $item);
						$item["id"] = $wpdb->insert_id;
						
						if ($result) {
							$message = "Item was successfully saved";
						} else {
							$notice = "There was an error while saving item";
						}
					} else {
						$result = $wpdb->update($this->table_executables, $item, ["id"=>$item["id"]]);
						
						if ($result) {
							$message = "Item was successfully updated";
						} else {
							var_dump( $wpdb->last_query );
							$notice = "There was an error while updating item";
						}
					}
				} else {
					$notice = $item_valid;
				}
			} else {
				$item = $default;
				
				if (isset($_REQUEST["id"])) {
					$item = $wpdb->get_row(
						$wpdb->prepare(
							"SELECT 
								* 
							FROM 
								{$this->table_executables} 
							WHERE 
								id = %d", $_REQUEST['id']
						), ARRAY_A
					);
					
					if (!$item) {
						$item   = $default;
						$notice = "Item not found";
					}
				}
			}

			add_meta_box(
				'executables_form_meta_box', //id
				'Fill executables details', //title
				[&$this,'executables_add_new_metabox'], //callback
				"", //screen
				'normal', //context
				'default' //priority
			);

			?>
			<div class="wrap">
				<div class="icon32 icon32-posts-post" id="icon-edit"><br></div>
				<h2><?=$item['id']==0?"Add New Executable":"Edit Executable"?> 
				<a class="add-new-h2" href="<?=get_admin_url(get_current_blog_id(), 'admin.php?page=runexeremotely_executables');?>">Back to list</a>
				<?php if ($item['id']!=0) : ?>
				<a class="add-new-h2" href="<?=get_site_url().'/wp-json/runexeremotely/exe/'.urlencode($item['name']) ?>" target="_blank">Link to data</a>
				<?php endif; ?>
				</h2>

				<?php if (!empty($notice)): ?>
				<div id="notice" class="error"><p><?=$notice ?></p></div>
				<?php endif;?>
				
				<?php if (!empty($message)): ?>
				<div id="message" class="updated"><p><?=$message ?></p></div>
				<?php endif;?>

				<form id="form" method="POST">
					<input type="hidden" name="nonce" value="<?=wp_create_nonce(basename(__FILE__))?>"/>
					<input type="hidden" name="id" value="<?=$item['id'] ?>"/>

					<div class="metabox-holder" id="poststuff">
						<div id="post-body">
							<div id="post-body-content">
								<?php do_meta_boxes('', 'normal', $item); ?>
								<input type="submit" value="Save" id="submit" class="button-primary" name="submit">
							</div>
						</div>
					</div>
				</form>
			</div>
			<?php
		}
		
		function display_executables_edit_validate($item) {
			$messages = [];

			if (!ctype_digit($item['id']))
				$messages[] = 'Invalid id';
			
			if (empty($item['name'])) 
				$messages[] = 'Name is required';
			
			if (empty($item['option_id']) || !ctype_digit($item['option_id']))
				$messages[] = 'Invalid option';

			if (empty($messages)) 
				return true;
			else
				return implode("<br />", $messages);
		}
		
		function executables_add_new_metabox($item) {
			global $wpdb;
			$options = $wpdb->get_results("SELECT * FROM {$this->table_options}", ARRAY_A);
			
			?>
			<table cellspacing="2" cellpadding="5" style="width: 100%;" class="form-table">
				<tbody>
				<tr class="form-field">
					<th valign="top" scope="row">
						<label for="name">Executable</label>
					</th>
					<td>
						<input id="name" name="name" type="text" style="" value="<?=esc_attr($item['name'])?>" class="code" placeholder="zen" required>
					</td>
				</tr>
				<tr class="form-field">
					<th valign="top" scope="row">
						<label for="option_id">Option</label>
					</th>
					<td>
						<select id="option_id" name="option_id" style="width: 95%">
						<?php						
						foreach ($options as $row) {
							?><option value="<?=$row["id"]?>" <?php if($item["option_id"]==$row["id"] || $row["id"]==0 && $row["argline"]=="?turnoff")echo 'selected="selected"';?>><?=esc_html($row["name"])?></option><?php
						}
						?>
						</select>
					</td>
				</tr>
				</tbody>
			</table>
			<?php
		}
	}
}

$RunExeRemotely = RunExeRemotely::getInstance();
