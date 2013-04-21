<?php
/**
 * @file
 * taxonomyblocks.admin.inc
 */

/**
 * List added blocks.
 */
function taxonomyblocks_list_page_callback() {
  $header = array(t('Block title'), t('Actions'));

  $result = db_select('tb_added_list', 'tb')
    ->fields('tb', array('tb_id', 'block_title', 'custom'))
    ->execute()
    ->fetchAll();
		
	// define $rows in case no taxonomy blocks have been defined.
	$rows = array();	
			
  foreach ($result as $tb_block) {
    drupal_add_library('contextual', 'contextual-links');

    $row = array($tb_block->block_title);

    $links = array(
      '#theme' => 'item_list',
      '#items' => array(
        l(t('Configure'), 'admin/config/taxonomyblocks/configure/' . $tb_block->tb_id),
        l(t('Reset'), 'admin/config/taxonomyblocks/reset/' . $tb_block->tb_id),
        l(t('Delete'), 'admin/config/taxonomyblocks/delete/' . $tb_block->tb_id)
      ),
      '#attributes' => array('class' => array('links', 'inline')),
    );

    $row[]  = drupal_render($links);
    $rows[] = $row;
  }

  return array(
    'table' => array(
      '#theme' => 'table',
      '#rows' => $rows,
      '#header' => $header,
    ),
    '#attached' => array(
      'css' => array(
        drupal_get_path('module', 'taxonomyblocks') . '/misc/taxonomyblocks.admin.css'
      ),
    ),
  );
}

/**
 * Simple form to create new block.
 */
function taxonomy_blocks_add_form($form, $form_state) {
  $form['title'] = array(
    '#type' => 'textfield',
    '#size' => '64',
    '#title' => t('New Taxonomy Block Title'),
    '#default_value' => '',
    '#description' => t('This will create a new block to which you will be able to add a single vocabulary or multiple vocabularies.'),
  );
  $form['taxonomy_blocks_add_form_submit'] = array(
    '#type' => 'submit',
    '#value' => t('Create New Block'),
  );
  return $form;
}

/**
 * Submit handler for taxonomy_blocks_add_form().
 */
function taxonomy_blocks_add_form_submit($form, &$formstate) {
  $title = check_plain($formstate['values']['title']);
  $tb_id = db_insert('tb_added_list')
            ->fields(array(
						  'block_title' => $title,
							))
            ->execute();
  drupal_set_message(t('Your block has been added'));
  drupal_goto('admin/config/taxonomyblocks/configure/' . $tb_id);
}

/**
 * Form to configure the taxonomy blocks.
 */
function taxonomyblocks_config_form($form, $form_state, $block) {
  $tb_id      = $block->tb_id;
  $custom     = $block->custom;
  $omit_tids  = implode(',', $block->omit_tids);
  $open_tids  = implode(',', $block->open_tids);
  $additional = variable_get('tb_additional', 0);

	// create items for form
  $form = array('#tree' => TRUE);
  $form['vocab'] = array(
    '#type' => 'fieldset',
    '#title' => t('Configuration Options'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#tree' => TRUE,
  );

  $form['vocab_options'] = array(
    '#type' => 'fieldset',
    '#title' => t('Tid Options'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#tree' => TRUE,
    '#description' => t('Omitted terms will only be hidden from this block. These settings have no affect on the actual taxonomies.'),
  );

  $form['main_title'] = array(
    '#type' => 'textfield',
    '#title' => 'Main Title for block',
    '#default_value' => $block->block_title,
  );

  $vocabs = db_query("SELECT vid, name, machine_name FROM {taxonomy_vocabulary}");
  foreach ($vocabs as $vocab) {
    $vocab_array[] = $vocab->vid;
    if ($custom) {
      $tb_customized = 'SELECT * FROM {tb_customized} WHERE tb_id = :tb_id and vid = :vid';
      $tb_customized = db_query($tb_customized, array(':tb_id' => $tb_id, ':vid' => $vocab->vid))->fetchObject();
      $enabled       = (isset($tb_customized->enabled) && $tb_customized->enabled == 1) ? TRUE : FALSE;
      $node_count    = !empty($tb_customized->counter) ? $tb_customized->counter : 'all';
      $path          = !empty($tb_customized->path_pattern) ? $tb_customized->path_pattern : 'taxonomy/term/%tid';
      $custom_title  = !empty($tb_customized->custom_title) ? $tb_customized->custom_title : check_plain($vocab->name);
      $weight        = !empty($tb_customized->weight) ? $tb_customized->weight : 0;
      $link_title    = !empty($tb_customized->title_pattern) ? $tb_customized->title_pattern : 'Content tagged with @termname';
    }
    else {
      $link_title   = 'Content tagged with @termname';
      $path         = 'taxonomy/term/%tid';
      $node_count   = 'all';
      $enabled      = TRUE;
      $custom_title = check_plain($vocab->name);
      $weight       = 0;
    }

    $content_types = array();
    $content_types['all'] = t('All Content Types');

    $form['vocab'][$vocab->vid] = array(
      '#type' => 'fieldset',
      '#title' => check_plain($vocab->name),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#tree' => TRUE,
    );
    $form['vocab'][$vocab->vid]['enable_vocab'] = array(
      '#type' => 'checkbox',
      '#title' => t('Enable'),
      '#default_value' => $enabled,
    );

    foreach (node_type_get_types() as $key => $val) {
      $content_types[$key] = $val->name;
    }

    $content_types['none'] = t("Don't show node count");
    $form['vocab'][$vocab->vid]['node_count'] = array(
      '#type' => 'select',
      '#title' => t('Content type options for node count'),
      '#default_value' => $node_count,
      '#options' => $content_types,
    );

    $children = FALSE;
    $empty = FALSE;
    if (is_array($additional) && isset($additional[$tb_id][$vocab->vid])) {
      $children = $additional[$tb_id][$vocab->vid]['children_count'];
      $empty = $additional[$tb_id][$vocab->vid]['empty'];
    }

    $form['vocab'][$vocab->vid]['children_count'] = array(
      '#type' => 'checkbox',
      '#title' => t('Show count of nodes belonging to children terms as well.'),
      '#default_value' => $children,
    );
    $form['vocab'][$vocab->vid]['empty'] = array(
      '#type' => 'checkbox',
      '#title' => t("Show terms that don't have nodes if at least one child term has a node."),
      '#default_value' => $empty,
    );

    $form['vocab'][$vocab->vid]['custom_title'] = array(
      '#type' => 'textfield',
      '#title' => t('Custom title for vocabulary'),
      '#size' => 40,
      '#default_value' => $custom_title,
    );
    $form['vocab'][$vocab->vid]['path_pattern'] = array(
      '#type' => 'textfield',
      '#title' => t('Path pattern'),
      '#size' => 40,
      '#default_value' => $path,
      '#required' => TRUE,
    );
    $form['vocab'][$vocab->vid]['title_pattern'] = array(
      '#type' => 'textfield',
      '#title' => t('Link title pattern'),
      '#size' => 40,
      '#default_value' => $link_title,
      '#required' => TRUE,
    );
    $form['vocab'][$vocab->vid]['weight'] = array(
      '#type' => 'weight',
      '#title' => t('Weight'),
      '#default_value' => $weight,
    );
    $form['vocab'][$vocab->vid]['reset'] = array(
      '#type' => 'checkbox',
      '#title' => t('Reset settings for this vocab'),
      '#default_value' => FALSE,
    );
    $form['vocab_options']['vocab'][$vocab->vid]= array(
      '#type' => 'fieldset',
      '#title' => check_plain($vocab->name),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#tree' => TRUE,
    );

    $sub_result = 'SELECT tid, name FROM {taxonomy_term_data} WHERE vid = :vid';
    $sub_result = db_query($sub_result, array(':vid' => $vocab->vid));
    foreach ($sub_result as $term) {
      $term_array[] = $term->tid;

      $form['vocab_options']['vocab'][$vocab->vid]['terms'][$term->tid] = array(
        'name' => array(
          '#type' => 'item',
          '#title' => check_plain($term->name),
        ),
        'omit' => array(
          '#type' => 'checkbox',
          '#default_value' => in_array($term->tid, explode(',', $omit_tids)),
        ),
        'open' => array(
          '#type' => 'checkbox',
          '#default_value' => in_array($term->tid, explode(',', $open_tids)),
        ),
      );
    }
    $form['vocab_options']['vocab'][$vocab->vid]['term_array']= array(
      '#type' => 'value',
      '#value' => $term_array,
    );
  }

  $form['reset_all'] = array(
    '#type' => 'checkbox',
    '#title' => 'Reset All',
    '#default_value' => FALSE,
  );
  $form['save_edit'] = array(
    '#type' => 'submit',
    '#value' => t('Save Configuration'),
  );

  $form['tb_id']  = array(
    '#type' => 'value',
    '#value' => $tb_id,
  );
  $form['vocab_array']  = array(
    '#type' => 'value',
    '#value' => $vocab_array,
  );

  $form['#theme'] = 'taxonomyblocks_config_form';

  return $form;
}

/**
 * Theming function for taxonomyblocks_config_form().
 */
function theme_taxonomyblocks_config_form($variables) {
  $form = &$variables['form'];
  $output  = drupal_render($form['main_title']);
  $output .= drupal_render($form['vocab']);

  if (!empty($form['vocab_array']['#value'])) {
    foreach ($form['vocab_array']['#value'] as $vid) {
      $header = array('term' => t('Term'), 'omit' => t('Omit'), 'expanded' => t('Expanded by default'));
      $rows = array();
      $form['vocab_options']['vocab'][$vid]['table']['#value'] = array();
      if (!empty($form['vocab_options']['vocab'][$vid]['term_array']['#value'])) {
        foreach ($form['vocab_options']['vocab'][$vid]['term_array']['#value'] as $tid) {
          $row = array();
          if (!empty($form['vocab_options']['vocab'][$vid]['terms'][$tid]['name'])) {
            $row[]  = drupal_render($form['vocab_options']['vocab'][$vid]['terms'][$tid]['name']);
          }
          if (!empty($form['vocab_options']['vocab'][$vid]['terms'][$tid]['omit'])) {
            $row[]  = drupal_render($form['vocab_options']['vocab'][$vid]['terms'][$tid]['omit']);
          }
          if (!empty($form['vocab_options']['vocab'][$vid]['terms'][$tid]['open'])) {
            $row[]  = drupal_render($form['vocab_options']['vocab'][$vid]['terms'][$tid]['open']);
          }
          if (!empty($row)) {
            $rows[] = $row;
          }
        }
      }

      $form['vocab_options']['vocab'][$vid]['table'] = array(
        '#type' => 'item',
        '#markup' => theme('table',  array('header' => $header, 'rows' => $rows))
      );
    }
  }
  $output .= drupal_render($form['vocab_options']);
  $output .= drupal_render($form['reset_all']);
  $output .= drupal_render($form['save_edit']);
  $output .= drupal_render_children($form);

  return $output;
}

function taxonomyblocks_config_form_submit($form, $formstate) {
  $additional = variable_get('tb_additional', array());
  $vid_array = $formstate['values']['vocab_array'];
  $omit_tids_checked = array();
  $open_tids_checked = array();
  if (!empty($vid_array)) {
    foreach ($vid_array as $vid) {
      $tid_array = $formstate['values']['vocab_options']['vocab'][$vid]['term_array'];
      if (!empty($tid_array)) {
        foreach ($tid_array as $tid) {
          if (isset($formstate['values']['vocab_options']['vocab'][$vid]['terms'][$tid]['omit']) && $formstate['values']['vocab_options']['vocab'][$vid]['terms'][$tid]['omit'] == TRUE) {
            $omit_tids_checked[] = $tid;
          }
          if (isset($formstate['values']['vocab_options']['vocab'][$vid]['terms'][$tid]['open']) && $formstate['values']['vocab_options']['vocab'][$vid]['terms'][$tid]['open'] == TRUE) {
            $open_tids_checked[] = $tid;
          }
        }
      }

    }
  }
  $tb_id = $formstate['values']['tb_id'];
  if ($formstate['values']['vocab'][$vid]['reset'] == FALSE) {
    $maintitle = $formstate['values']['main_title'];
    db_update('tb_added_list')
      ->fields(array('block_title' => $maintitle, ))
      ->condition('tb_id', $tb_id, '=')
      ->execute();
  }

  if (!empty($vid_array)) {
    foreach ($vid_array as $vid) {
      if ($formstate['values']['vocab'][$vid]['reset'] == FALSE) {
        $enabled_array[] = $vid;
        $titlepattern = '';
        $pathpattern  = '';
        $node_count   = '';
        $weight       = '';
        if ($formstate['values']['vocab'][$vid]['custom_title'] != '<none>') {
          $customtitle  = check_plain($formstate['values']['vocab'][$vid]['custom_title']);
        }
        else {
          $customtitle  =  $formstate['values']['vocab'][$vid]['custom_title'];
        }
        $titlepattern = check_plain($formstate['values']['vocab'][$vid]['title_pattern']);
        $pathpattern  = check_plain($formstate['values']['vocab'][$vid]['path_pattern']);
        $node_count   = $formstate['values']['vocab'][$vid]['node_count'];
        $empty        = $formstate['values']['vocab'][$vid]['empty'];
        $ch_count     = $formstate['values']['vocab'][$vid]['children_count'];
        $additional[$tb_id][$vid]['children_count'] = $ch_count;
        $additional[$tb_id][$vid]['empty'] = $empty;
        $weight       = $formstate['values']['vocab'][$vid]['weight'];
        $enabled      = $formstate['values']['vocab'][$vid]['enable_vocab'];
        if ($enabled == TRUE) {
          $enabled = 1;
        }
        else {
          $enabled = 0;
        }

        db_update('tb_added_list')
          ->fields(array('custom' => 1, ))
          ->condition('tb_id', $tb_id, '=')
          ->execute();

        db_delete('tb_customized')
          ->condition('tb_id', $tb_id)
          ->condition('vid', $vid)
          ->execute();

        db_insert('tb_customized')
          ->fields(array(
            'tb_id' => $tb_id,
            'vid' => $vid,
            'custom_title' => $customtitle,
            'path_pattern' => $pathpattern,
            'title_pattern' => $titlepattern,
            'counter' => $node_count,
            'weight' => $weight,
            'enabled' => $enabled,
          ))
          ->execute();

        variable_set('tb_additional', $additional);
      }
      elseif ($formstate['values']['vocab'][$vid]['reset'] == TRUE) {
        db_delete('tb_customized')
          ->condition('tb_id', $tb_id)
          ->condition('vid', $vid)
          ->execute();

        $additional[$tb_id][$vid]['empty'] = array();
        $additional[$tb_id][$vid]['children_count'] = array();
        variable_set('tb_additional', $additional);
      }
    }
    $omit_tids = implode(',', $omit_tids_checked);
    $open_tids = implode(',', $open_tids_checked);
    db_update('tb_added_list')
      ->fields(array(
      'omit_tids' => $omit_tids,
      'open_tids' => $open_tids,
      'custom' => 1,
    ))
    ->condition('tb_id', $tb_id, '=')
    ->execute();

    if ($formstate['values']['reset_all'] == TRUE) {
      $token = drupal_get_token('tb_del');
      taxonomy_blocks_reset($tb_id . '-' . $token);
      $additional[$tb_id] = array();
      variable_set('tb_additional', $additional);
    }
  }
  drupal_set_message(t('Your changes have been saved'), $type = 'status', $repeat = FALSE );
  drupal_goto('admin/config/taxonomyblocks');
}

/**
 * Form to delete a taxonomy block.
 */
function taxonomy_blocks_delete_form($form, $form_state, $block) {
  $form['#block'] = $block;
  $form['#redirect'] = 'admin/config/taxonomyblocks';
  $question = t('Are you sure to delete this block?');
  $path = 'admin/config/taxonomyblocks';
  return confirm_form($form, check_plain($question), $path);
}

function taxonomy_blocks_delete_form_submit($form, $form_state) {
  $tb_id = $form['#block']->tb_id;

  db_delete('tb_customized')
    ->condition('tb_id', $tb_id)
    ->execute();

  db_delete('tb_added_list')
    ->condition('tb_id', $tb_id)
    ->execute();

  db_delete('block')
    ->condition('module', 'taxonomyblocks')
    ->condition('delta', $tb_id)
    ->execute();

  drupal_set_message(t('The block has been deleted'));
  drupal_goto('admin/config/taxonomyblocks');
}

/**
 *
 *
 * @param type $tb_id_token
 */
function taxonomy_blocks_reset_form($form, $form_state, $block) {
  $form['#block'] = $block;
  $form['#redirect'] = 'admin/config/taxonomyblocks';
  $question = t('Are you sure to reset this block?');
  $path = 'admin/config/taxonomyblocks';
  return confirm_form($form, check_plain($question), $path);
}

function taxonomy_blocks_reset_form_submit($form, $form_state, $block) {
  $tb_id = $form['#block']->tb_id;

  db_delete('tb_customized')
    ->condition('tb_id', $tb_id)
    ->execute();

  db_update('tb_added_list')
    ->fields(array('custom' => 0))
    ->condition('tb_id', $tb_id)
    ->execute();

  drupal_set_message(t('The block has been reset'));
  drupal_goto('admin/config/taxonomyblocks');
}