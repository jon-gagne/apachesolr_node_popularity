<?php

/**
 * @file
 * Admin page callbacks for the Apache Solr Popularity module.
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
    array('data' => t('Popularity'), 'field' => 'popularity', 'sort' => 'desc'),
    array('data' => t('Page'), 'field' => 'nid'),
  );

  $query = db_select('popularity_counter', 'p', array('target' => 'slave'))->extend('PagerDefault')->extend('TableSort');;
  //$query->addExpression('COUNT(path)', 'hits');
  $query->fields('p', array('popularity', 'nid'));
  $query
    ->groupBy('nid')
    ->limit(30)
    ->orderByHeader($header);
  $result = $query->execute();
  $rows = array();
  foreach ($result as $page) {
    $rows[] = array($page->popularity, $page->nid);
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
  $form['settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Apache Solr Popularity Settings'),
  );
  $form['settings']['popularity_tracker'] = array(
    '#type' => 'checkbox',
    '#title' => t('Track node popularity'),
    '#default_value' => variable_get('popularity_tracker', 0),
    '#description' => t('Track node viewing and calculate a popularity metric based on view frequency.'),
  );

  return system_settings_form($form);
}

