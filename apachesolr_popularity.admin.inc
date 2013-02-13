<?php

/**
 * @file
 * Admin page callbacks for the Apache Solr Popularity module.
 * 
 * @author Jonathan Gagne (jongagne)
 *   @see drupal.org/user/2409764
 */

/**
 * Page callback: Displays popularity of the the top pages.
 *
 * This displays the pages with the highest popularity.
 * Note: only updates ones per cron run
 *
 * @return
 *   A render array containing information about the most popular pages.
 */
function apachesolr_popularity_top_pages() {
  
  $header = array(
    array('data' => t('Title'), 'field' => 'nid'),
    //array('data' => t('Node ID'), 'field' => 'nid'),
    array('data' => t('Popularity'), 'field' => 'popularity', 'sort' => 'desc'),
    array('data' => t('Recent Count'), 'field' => 'recentcount', 'sort' => 'desc'),
    array('data' => t('Days Tracked'), 'field' => 'daystracked'),
  );

  $query = db_select('apachesolr_popularity_counter', 'p', array('target' => 'slave'))->extend('PagerDefault')->extend('TableSort');;
  //$query->addExpression('COUNT(path)', 'hits');
  $query->fields('p', array('nid', 'popularity', 'recentcount', 'daystracked'));
  $query
    ->groupBy('nid')
    ->limit(30)
    ->orderByHeader($header)
    ->orderBy('popularity', 'desc')
    ->orderBy('recentcount', 'desc');
  
  $result = $query->execute();
  $rows = array();
  foreach ($result as $page) {
    $rows[] = array(_apachesolr_popularity_format_item($page->nid), /*$page->nid,*/ $page->popularity, $page->recentcount, $page->daystracked);
  }
  //TODO: Change nid to a link to the path
  
  drupal_set_title(t('Most popular pages'), PASS_THROUGH);
  $build['popularity_top_pages_table'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $rows,
    '#empty' => t('No popularity statistics available.'),
  );
  $build['popularity_top_pages_pager'] = array('#theme' => 'pager');
  return $build;
}


/**
 * Form constructor for the Apache Solr Popularity administration form.
 */
function apachesolr_popularity_settings_form() {
  // Config page
  
  // reloads Solr cache
  $default_env_id = variable_get('apachesolr_default_environment', 'solr');
  $default_solr_env = db_query('SELECT * FROM {apachesolr_environment} WHERE env_id = :env_id', array(':env_id' => $default_env_id))->fetchObject();
  if (isset($default_solr_env->name) != '' AND isset($default_solr_env->url)) {
    $solr_description = t("<i>Default") . " Solr " . t('server name: </i>'). $default_solr_env->name . t('<br><i>URL: </i>') . $default_solr_env->url .'<br><br>';
  }
  else {
    $solr_description = t('<strong>* Error locating default Solr server.  ') . l(t('Check Apache Solr Settings'), 'admin/config/search/apachesolr/settings/') . ' *</strong><br><br>';
  }    
    
  
  $form['solr_settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Solr Settings'),
    '#description' => $solr_description,
  );
  $form['solr_settings']['popularity_tracker'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable Apache Solr Node Popularity'),
    '#default_value' => variable_get('popularity_tracker', 0),
    '#description' => t('Track node viewing and calculate a popularity metric based on view frequency.'),
  ); 
          
  $form['solr_settings']['apachesolr_popularity_solr_data_dir'] = array(
    '#type' => 'textfield',
    '#title' => t("Path of the default Solr server's data directory (must be writtable)"),
    '#default_value' => variable_get('apachesolr_popularity_solr_data_dir', ''),
    '#description' => 'e.g.  /opt/tomcat/solr/data/',
    '#maxlength' => 256,
    '#required' => TRUE
  );
  
  $form['basic_config'] = array(
    '#type' => 'fieldset',
    '#title' => t('Basic popularity configurations'),
  );
  $popularity_influence_options = array( '0.01' => '1%', '0.1' => '10%', '0.5' => '50%', '1' => '100%', '2' => '200%', '4' => '400%', '10' => '1000%' );
  $form['basic_config']['apachesolr_popularity_influence'] = array(
      '#type' => 'select',
      '#title' => t('Popularity influence'),
      //'#default_value' => array_search($popularity_influence, $popularity_influence_options), //,
      '#default_value' => (string) variable_get('apachesolr_popularity_influence', '2'),
      '#options' => $popularity_influence_options,
      '#description' => t("Amount that node popularity influences Apache Solr's ranking scores.<br>Ranking scores will be increase up to this amount."),
      '#states' => array(
        'enabled' => array(
          ':input[name="popularity_tracker"]' => array('checked' => TRUE),
          ':input[name="apachesolr_popularity_solr_data_dir"]' => array('empty' => FALSE)
        ),
      ),
  );
  
  //$forgetting_window = variable_get('apachesolr_popularity_forgetting_window', 5.0);
  $forgetting_window_options = array( '0.1' => '0.1', '0.5' => '0.5', '2' => '2', '5' => '5', '10' => '10', '20' => '20', '100' => '100', '1000' => '1000' );
  $form['basic_config']['apachesolr_popularity_forgetting_window'] = array(
      '#type' => 'select',
      '#title' => t('Forgetting window'),
      //'#default_value' => array_search($forgetting_window, $forgetting_window_options),
      '#default_value' => (string) variable_get('apachesolr_popularity_forgetting_window', '5'),
      '#options' => $forgetting_window_options,
      '#description' => t('After this many days of tracking, biasing recent popularity begins to fade on. <br> The lower the value is, the more that newer popularity measurements are biased.'),
      '#states' => array(
        'enabled' => array(
          ':input[name="popularity_tracker"]' => array('checked' => TRUE),
          ':input[name="apachesolr_popularity_solr_data_dir"]' => array('empty' => FALSE)
        ),
      ),
  );
  
  $form['reset_fieldset'] = array(
    '#type' => 'fieldset',
    '#title' => t('Clear popularity values'),
  );
  $form['reset_fieldset']['reset_tracker'] = array(
    '#type' => 'checkbox',
    '#title' => t('Reset popularity values for all nodes.'),
    '#default_value' => FALSE, //variable_get('reset_tracker', FALSE),
    '#states' => array(
        'enabled' => array(
          ':input[name="popularity_tracker"]' => array('checked' => TRUE),
          ':input[name="apachesolr_popularity_solr_data_dir"]' => array('empty' => FALSE)
        ),
      ),
  );
  
  $form['reset_fieldset']['confirm_reset_fieldset'] = array(
    '#type' => 'fieldset',
    //'#title' => t('Confirm reset? (Cannot be undone)'),
    '#states' => array(
      'visible' => array(
        ':input[name="reset_tracker"]' => array('checked' => TRUE)
       ),
    ),
  );
  $form['reset_fieldset']['confirm_reset_fieldset']['confirm_reset'] = array(
    '#type' => 'checkbox',
    '#title' => t('Confirm reset? (Cannot be undone)'),
    '#default_value' => FALSE,
    '#states' => array(
      'unchecked' => array(
        ':input[name="reset_tracker"]' => array('checked' => TRUE)
       ),
      'visible' => array(
        ':input[name="reset_tracker"]' => array('checked' => TRUE)
       ),
    ),
  );
  $form['#submit'][] = 'apachesolr_popularity_settings_form_submit';
  
  return system_settings_form($form);
}


/**
 * Validate setting form - hook_form_validate
 */
function apachesolr_popularity_settings_form_validate($form, &$form_state) {
  
  if ( $form_state['values']['reset_tracker']==1 AND 
       $form_state['values']['confirm_reset']==0) {
    form_set_error('confirm_reset', t('Please confirm reset!'));
  }
  
  $dir = $form_state['values']['apachesolr_popularity_solr_data_dir'];
  if ( $dir != '' ) {
    if ( !file_exists($dir) ) form_set_error('apachesolr_popularity_solr_data_dir', t('Directory does not exist'));

    if ( !is_writable($dir) ) form_set_error('apachesolr_popularity_solr_data_dir', t('Directory does is not writable'));

    // Adds slash if not present
    if ( $dir[strlen($dir)-1] != '\\' AND $dir[strlen($dir)-1] != '/' )
      $form_state['values']['apachesolr_popularity_solr_data_dir'] .= DIRECTORY_SEPARATOR; 
  }
}



/**
 * Process form - hook_form_submit
 */
function apachesolr_popularity_settings_form_submit($form, &$form_state) {
  
  // whether to update solr with popularity data
  $send_to_solr = false;
  
  // Sets up popularity tracking table
  if ( ($form_state['values']['popularity_tracker']==1) AND (variable_get('popularity_tracker')==0) ) {
    _track_new_nodes();
    $send_to_solr = true;
  }
   
  // Checks if Solr data directory has changed.  If yes, than change it now before other data is sent to Solr
  if ( $form_state['values']['apachesolr_popularity_solr_data_dir'] != variable_get('apachesolr_popularity_solr_data_dir') ) {
    variable_set('apachesolr_popularity_solr_data_dir', $form_state['values']['apachesolr_popularity_solr_data_dir']);  
  }      
   
  // Checks if popularity influence has changed.  If yes, resend popularity to Solr
  if ( $form_state['values']['apachesolr_popularity_influence'] != variable_get('apachesolr_popularity_influence') ) {
    variable_set('apachesolr_popularity_influence', $form_state['values']['apachesolr_popularity_influence']);    
    $send_to_solr = true;
  }    
  
  // Checks if forgetting window has changed
  if ( $form_state['values']['apachesolr_popularity_forgetting_window'] != variable_get('apachesolr_popularity_forgetting_window') ) {
    drupal_set_message(t("Forgetting window changed.  This will effect future popularity values only."));
  }
    
  // Resets popularity tracking
  if ( $form_state['values']['reset_tracker']==1 AND $form_state['values']['confirm_reset']==1) {
    _reset_popularity();
    $send_to_solr = true;
  }  
  
    
  // Check if popularity should be sent to solr
  if ( $send_to_solr == true ) {   
    $dir = variable_get('apachesolr_popularity_solr_data_dir');
    if ( $dir == '' OR !file_exists($dir) OR !is_writable($dir) ) {       
      drupal_set_message(t('Solr data directory'));
    }
    else {
      drupal_set_message(t("Popularity data sent to Apache Solr."));
      _apachesolr_popularity_send_to_solr();
    }
  }
}


/**
 * Form constructor for the Apache Solr Popularity advanced administration form.
 */
function apachesolr_popularity_advanced_form() {
  
  $form_changed = false;
  
  // Config page
  $form['settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Advanced Popularity Settings'),
  );
  
  if ( variable_get('popularity_tracker')==1 ) {
    $form['settings']['apachesolr_popularity_enable_advanced'] = array(
      '#type' => 'checkbox',
      '#title' => t('Enables advanced settings'),
      '#default_value' => variable_get('apachesolr_popularity_enable_advanced', FALSE),     
      '#description' => t('For advanced users only'),
    );
  }
  else {
    drupal_set_message(l(t('Enable node popularity first'), 'admin/config/search/apachesolr/apachesolr_popularity'), 'error');
    $form['settings']['apachesolr_popularity_enable_advanced'] = array(
      '#type' => 'checkbox',
      '#title' => t('Enables advanced settings'),
      '#default_value' => FALSE,
      '#disabled' => TRUE,
      '#description' => t('For advanced users only'),
    );    
  }  
  
  //$init_damping = variable_get('apachesolr_popularity_init_damping', 0.2);
  $init_damping_options = array( '0' => '0.0', '0.05' => '0.05', '0.2' => '0.2', '0.5' => '0.5', '1' => '1', '2' => '2', '5' => '5' );
  $form['settings']['apachesolr_popularity_init_damping'] = array(
      '#type' => 'select',
      '#title' => t('Initial dampening'),
      //'#default_value' => array_search($init_damping, $init_damping_options), 
      '#default_value' => (string) variable_get('apachesolr_popularity_init_damping', '0.2'),
      '#options' => $init_damping_options,
      '#description' => t("Smoothes initial popularity estimates.<br> 0 = None, 0.2 = Moderate, 2 = High."),
      '#states' => array(
        'disabled' => array(
          ':input[name="apachesolr_popularity_enable_advanced"]' => array('checked' => FALSE)
        ),
      ),
  );
  
  //$init_popularity = variable_get('apachesolr_popularity_init_popularity', 20);
  $init_popularity_options = array( '0' => '0', '10' => '10', '20' => '20', '50' => '50', '100' => '100' );
  $form['settings']['apachesolr_popularity_init_popularity'] = array(
      '#type' => 'select',
      '#title' => t('Initial node popularity'),
      //'#default_value' => array_search($init_popularity, $init_popularity_options), //,
      '#default_value' => (string) variable_get('apachesolr_popularity_init_popularity', '20'),
      '#options' => $init_popularity_options,
      '#description' => t("Sets inital popularity for newly tracked nodes.<br>Starting popularity: 0 = Lowest, 20 = Moderate, 100 = Highest."),
      '#states' => array(
        'disabled' => array(
          ':input[name="apachesolr_popularity_enable_advanced"]' => array('checked' => FALSE)
        ),
      ),
  );
    
  //$compression = variable_get('apachesolr_popularity_compression', 'Sqrt');
  $compression_options = array( 'N' => 'None', 'S' => 'Sqrt', 'L' => 'Log' );
  $form['settings']['apachesolr_popularity_compression'] = array(
      '#type' => 'select',
      '#title' => t('Popularity compression on high traffic nodes'),
      //'#default_value' => array_search($compression, $compression_options), //,
      '#default_value' => (string) variable_get('apachesolr_popularity_compression', 'N'),
      '#options' => $compression_options,
      '#description' => t("Prevent high traffic nodes from dominating the search results.<br>Compression type: Linear = None, Sqrt = Moderate, Log = High."),
      '#states' => array(
        'disabled' => array(
          ':input[name="apachesolr_popularity_enable_advanced"]' => array('checked' => FALSE)
        ),
      ),
  );
    
  //$low_pop_influence = variable_get('apachesolr_popularity_low_pop_influence', 1);
  $low_pop_influence_options = array( '0' => '0', '0.2' => '0.2', '0.5' => '0.5', '1' => '1', '2' => '2' );
  $form['settings']['apachesolr_popularity_low_pop_influence'] = array(
      '#type' => 'select',
      '#title' => t('Low popularity influence'),
      //'#default_value' => array_search($low_pop_influence, $low_pop_influence_options), //,
      '#default_value' => (string) variable_get('apachesolr_popularity_low_pop_influence', '1'),
      '#options' => $low_pop_influence_options,
      '#description' => t("Allows unpopular pages to still still be found in a search.<br> 0 = Hard to find unpopular page, 1 = Typical, 2 = Found easier."),
      '#states' => array(
        'disabled' => array(
          ':input[name="apachesolr_popularity_enable_advanced"]' => array('checked' => FALSE)
        ),
      ),
  );
  
  $form['#submit'][] = 'apachesolr_popularity_advanced_form_submit';
  $form['submit']['#disabled'] = TRUE;
  
  return system_settings_form($form);
}


/**
 * Process form - hook_form_submit
 */
function apachesolr_popularity_advanced_form_submit($form, &$form_state) {

      
  // Checks if initial damping has changed
  if ( $form_state['values']['apachesolr_popularity_init_damping'] != variable_get('apachesolr_popularity_init_damping') ) {
    drupal_set_message(t("Initial damping changed.  This will effect future popularity values only."));
  }
    
  // Checks if initial popularity has changed
  if ( $form_state['values']['apachesolr_popularity_init_popularity'] != variable_get('apachesolr_popularity_init_popularity') ) {
    drupal_set_message(t("Initial node popularity changed.  This will effect future popularity values only."));
  }
  
  $send_to_solr = false;
  
  // Checks if popularity compression has changed.  Is yes, send data to Solr
  if ( $form_state['values']['apachesolr_popularity_compression'] != variable_get('apachesolr_popularity_compression') ) {
    variable_set('apachesolr_popularity_compression', $form_state['values']['apachesolr_popularity_compression']);
    $send_to_solr = true;
  }
  
  // Checks if low popularity influence has changed. 
  if ( $form_state['values']['apachesolr_popularity_low_pop_influence'] != variable_get('apachesolr_popularity_low_pop_influence') ) {
    variable_set('apachesolr_popularity_low_pop_influence', $form_state['values']['apachesolr_popularity_low_pop_influence']);
    $send_to_solr = true;
  }
  
  // Check if popularity should be sent to solr
  if ( $send_to_solr == true ) {   
    $dir = variable_get('apachesolr_popularity_solr_data_dir');
    if ( $dir == '' OR !file_exists($dir) OR !is_writable($dir) ) {       
      drupal_set_message('Solr ' . l(t('data directory'), 'admin/config/search/apachesolr/apachesolr_popularity') . t(' not properly set'), 'error');
    }
    else {
      drupal_set_message(t("Popularity data sent to Apache Solr."));
      _apachesolr_popularity_send_to_solr();
    }
  }
}


/**
 * Track all nodes not currently being tracked
 */
function _track_new_nodes() {
  
  $node_create_list = db_query(
            'SELECT n.nid
             FROM {node} AS n
             LEFT JOIN {apachesolr_popularity_counter} AS apc
             ON n.nid=apc.nid
             WHERE apc.nid IS NULL'
            );

  $popularity = (float) variable_get('apachesolr_popularity_init_popularity', 20.0);
  $query = db_insert('apachesolr_popularity_counter')->fields(array('nid'));
  foreach ($node_create_list as $node) {
    $query->values(array(
          'nid' => $node->nid,
          'popularity' => $popularity
        ));
  }
  $query->execute();    
}


/**
 * Resets node popularities
 */
function _reset_popularity() {
  $popularity = (float) variable_get('apachesolr_popularity_init_popularity', 20.0);
  db_update('apachesolr_popularity_counter')
        ->fields(array(
                'popularity' => $popularity,
                'recentcount' => 0,
                'daystracked' => 0.0,
                )
        )
        ->execute();    
}

