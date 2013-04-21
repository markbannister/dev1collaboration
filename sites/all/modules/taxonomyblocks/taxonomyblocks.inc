<?php
/*
 * @file
 * Includes necessary functions for taxonomy blocks
 */

/**
 * Theme function for taxonomyblocks_tree.
 *
 * @see taxonomyblocks_block_view().
 */
function theme_taxonomyblocks_tree($variables) {
  global $user;

  $block  = $variables['element']['#block'];
  $tb_id  = $block->tb_id;
  $custom = $block->custom ? $block->custom : 'all';
  $omit   = $block->omit_tids;
  $open   = $block->open_tids;
  $uid    = $user->uid;
  $path   = drupal_get_path('module', 'taxonomyblocks');
  $extra  = variable_get('tb_additional', 0);

  if (arg(0) == 'node' && is_numeric(arg(1))) {
    $nid      = arg(1);
    $node     = node_load($nid);
    $nodeuid  = $node->uid;
    $nodetype = $node->type;
  }

  if (!empty($block->data)) {
    foreach ($block->data as $data) {
      $vid    = $data->vid;
      $_nodes = array();

      // @TODO: This is expensive function.
      $hasterms = count(taxonomy_get_tree($vid));
      if (!empty($hasterms)) {
        $nodecounter = $data->counter;
        foreach (taxonomyblocks_taxonomy_get_toplevel($vid) as $tid) {
          $_nodes[] = taxonomyblocks_term_has_nodes($tid, $nodecounter);
        }
      }

      if (!empty($_nodes)) {
        $vids[$vid] = $data;
      }
    }
  }

  $output = '';
  if (!empty($vids)) {
    $multiple = sizeof($vids) > 1;

    foreach ($vids as $vid => $data) {
      if (!$toplevel = taxonomyblocks_taxonomy_get_toplevel($vid)) {
        continue;
      }

      if ($custom) {
        $pathpattern  = $data->path_pattern;
        $nodecounter  = $data->counter;
        $titlepattern = $data->title_pattern;
      }
      else {
        $pathpattern  = 'taxonomy/term/%tid';
        $nodecounter  = 'all';
        $titlepattern = 'Content tagged with @termname';
      }

      $options = array(
        'pathpattern'  => $pathpattern,
        'nodecounter'  => $nodecounter,
        'titlepattern' => $titlepattern,
        'omit'         => $omit,
        'open'         => $open,
        'vid'          => $vid,
        'vocabname'    => taxonomy_vocabulary_load($vid)->name,
      );

      if (isset($nid)) {
        $options['nid']      = $nid;
      }
      if ($uid) {
        $options['useruid']  = $uid;
      }
      if (isset($nodeuid)) {
        $options['nodeuid']  = $nodeuid;
      }
      if (isset($nodetype)) {
        $options['nodetype'] = $nodetype;
      }

      if (is_array($extra)) {
        $options['empty']          = $extra[$tb_id][$vid]['empty'];
        $options['children_count'] = $extra[$tb_id][$vid]['children_count'];
      }

      $tree  = taxonomyblocks_jq_taxonomy_tree_creator($vid, $options);
      $trail = taxonomy_trail_creator($pathpattern);
      $menu  = theme('jquerymenu_menu', array('tree' => $tree, 'trail' => $trail));

      if ($multiple) {
        $vocabname = $custom ? $data->custom_title : taxonomy_vocabulary_load($vid)->name;

        if (($vocabname != '<none>') && !empty($menu)) {
          if (function_exists('tt')) {
            $vocabname = tt('taxonomy:vocabulary:' . $vid . ':name', $vocabname);
          }

          $output .= '<h3>' . $vocabname . '</h3>';
        }
      }
      $output .= $menu;
    }
  }
  return $output;
}

function taxonomyblocks_term_has_parent($tid) {
  $sql = 'SELECT parent FROM {taxonomy_term_hierarchy} WHERE tid = :tid';
  return db_query($sql, array(':tid' => $tid))->fetchField();
}

function taxonomyblocks_vocab_edit_link($vid) {
  if ($vocab = taxonomy_vocabulary_load($vid)) {
    return implode(' | ', array(
      l(t('Rearrange Terms'), "admin/structure/taxonomy/{$vocab->machine_name}"),
      l(t('Edit Vocabulary'), "admin/structure/taxonomy/{$vocab->machine_name}/edit")
    ));
  }
}

function taxonomyblocks_jq_taxonomy_tree_creator($vid, $options) {
  $toplevel = taxonomyblocks_taxonomy_get_toplevel($vid);
  if ($toplevel != NULL) {
    return taxonomyblocks_taxonomy_recursive_array_builder($toplevel, $options);
  }
}

function taxonomyblocks_taxonomy_recursive_array_builder($tid_array, $options) {
  if (!empty($options)) {
    $pathpattern   = $options['pathpattern'];
    $nodecounter   = !empty($options['nodecounter']) ? $options['nodecounter'] : 'all';
    $ch_count      = !empty($options['children_count']) ? $options['children_count'] : 0;
    $empty         = !empty($options['empty']) ? $options['empty'] : 0;
    $titlepattern  = !empty($options['titlepattern']) ? $options['titlepattern'] : NULL;
    $omit          = !empty($options['omit']) ? $options['omit'] : NULL;
    $open          = !empty($options['open']) ? $options['open'] : NULL;
    $vid           = !empty($options['vid']) ? $options['vid'] : NULL;
    $nid           = !empty($options['nid']) ? $options['nid'] : NULL;
    $vocabname     = !empty($options['vocabname']) ? $options['vocabname'] : NULL;
    $useruid       = !empty($options['useruid']) ? $options['useruid'] : NULL;
    $nodeuid       = !empty($options['nodeuid']) ? $options['nodeuid'] : NULL;
    $nodetype      = !empty($options['nodetype']) ? $options['nodetype'] : NULL;
  }
  foreach ($tid_array as $tid) {

    $termname =  taxonomy_term_load($tid)->name;
    if (function_exists('tt')) {
      $termname = tt('taxonomy:term:' . $tid . ':name', $termname);
    }
    $tree[$termname]['link']['tid'] = $tid;
    $tree[$termname]['link']['path_pattern'] = $pathpattern;
    $tree[$termname]['link']['editaccess'] = 'administer taxonomy';
    $tree[$termname]['link']['edit_path'] = 'taxonomy/term/' . $tid . '/edit';
    $tree[$termname]['link']['edit_text'] = t('Edit this term');
    $hasnodes = '';
    $hasnodes = taxonomyblocks_term_has_nodes($tid, $nodecounter);

    $count = '';

    $path  = str_replace('%tid', $tid, $pathpattern);
    $path  = str_replace('%termname', $termname, $path);
    if ($nid) {
      $path  = str_replace('%nid', $nid, $path);
      $path  = str_replace('%nodeuid', $nodeuid, $path);
      $path  = str_replace('%nodetype', $nodetype, $path);
    }
    $path  = str_replace('%vid', $vid, $path);
    if (!empty($useruid)) {
      $path  = str_replace('%useruid', $useruid, $path);
    }
    $path  = str_replace('%vocabname', $vocabname, $path);

    $sp = "/(?<=%arg\()\d(?=\))/";
    preg_match($sp, $path, $matches);
    if (!empty($matches)) {
      foreach ($matches as $match) {
        $arg = '';
        $arg = arg($match);
        $path = str_replace('%arg(' . $match . ')', $arg, $path);
      }
    }

    if ($nodecounter == 'none') {
      $tree[$termname]['link']['title'] = $termname ;
    }
    if ($nodecounter != 'none') {
      $count = taxonomyblocks_node_counter($tid, $nodecounter, $ch_count);
      $tree[$termname]['link']['title'] = $termname . ' (' . $count . ')';
    }


    $allchildren = taxonomyblocks_get_all_children($tid);
    $child_with_node_array = array();
    if (!empty($allchildren)) {
      $child_has_node = array();
      foreach ($allchildren as $ctid) {
        $childnode = taxonomyblocks_term_has_nodes($ctid, $nodecounter);
        if ($childnode != FALSE) {
          $child_has_node[] = $childnode;
        }
        if (!empty($omit)) {
          $child_is_omited = in_array($ctid, $omit);
        }
        else {
          $child_is_omited = FALSE;
        }

        $child_with_node_array = array();
        if (!empty($child_has_node) && $child_is_omited == FALSE) {
          $child_with_node_array[] = $ctid;
        }
      }
    }
    $omited = FALSE;
    if ($empty == 0) {
      if (empty($hasnodes)) {
        $omited = TRUE;
      }
      if (empty($omit) && !empty($hasnodes)) {
        $omited = FALSE;
      }
    }

    if ($empty == 1) {
      if ((empty($hasnodes) && empty($child_with_node_array))) {
        $omited = TRUE;
      }
      if (!empty($hasnodes) || !empty($child_with_node_array)) {
        $omited = FALSE;
      }
    }
    if (!empty($omit)) {
      if (in_array($tid, $omit)) {
        $omited = TRUE;
      }
    }
    if ($omited == TRUE) {
      $tree[$termname]['link']['hidden'] = '1';
    }
    else {
      $tree[$termname]['link']['hidden'] = '0';
    }
    $tree[$termname]['link']['href'] = $path;



    if (empty($titlepattern)) {
      $tree[$termname]['link']['options']['attributes']['title'] = t('Content tagged with @termname', array('@termname' => $termname));
    }
    else {
      $tree[$termname]['link']['options']['attributes']['title'] = t($titlepattern, array('@termname' => $termname));
    }
    $children = array();
    $tree[$termname]['below'] = array();
    $children = taxonomyblocks_get_one_level_children($tid);



    if (!empty($child_with_node_array)) {
        $tree[$termname]['link']['has_children'] = '1';

        if (!empty($open)) {
          if (in_array($tid, $open)) {
            $tree[$termname]['link']['expanded'] = 1;
          }
        }
      }

      else {
        $tree[$termname]['link']['has_children'] = '0';
      }
    if (!empty($children)) {
      $subarray = taxonomyblocks_taxonomy_recursive_array_builder($children, $options);
      $tree[$termname]['below'] = array_merge($tree[$termname]['below'], $subarray);
    }
    if (empty($children)) {
      $tree[$termname]['below'] = FALSE;
      $tree[$termname]['link']['has_children'] = '0';
    }
  }

  return $tree;
}


function taxonomy_trail_creator($pathpattern) {
  $pattern_array  = explode('/', $pathpattern);
  $p = 0;
  foreach ($pattern_array as $chunk) {
    if ($chunk != '%tid') {
      $p++;
    }
  }
  $pagetid = "";
  $pagetid = arg($p);
  $ancestors  = taxonomyblocks_get_all_ancestor_csv($pagetid);
  $ancestors = explode(',', $ancestors);
  foreach ($ancestors as $tid) {
    $term = str_replace('%tid', $tid, $pathpattern);
    $url_array[] = $term;
  }
  return $url_array;
}


function taxonomyblocks_get_all_ancestor_csv($tid) {
  $parent = '';
  $string = '';
  $parent = taxonomyblocks_term_has_parent($tid);
  if ($parent != '0') {
    $string .= $parent;
    $nextlevel = '';
    $nextlevel = taxonomyblocks_get_all_ancestor_csv($parent);
    if (!empty($nextlevel)) {
      $string .= ',' . $nextlevel;
    }
  }
  return $string;
}

function taxonomyblocks_node_counter($tid, $nodecounter = 'all', $ch_count = 0) {
  $count = 0;
  if ($ch_count == 0) {
    if ($nodecounter != 'all') {
      $query = new EntityFieldQuery();
      $query->entityCondition('entity_type', 'node')
        ->entityCondition('bundle', $nodecounter)
        ->propertyCondition('status', 1);

      $result = $query->execute();
      if (isset($result['node'])) {
        $nids = array_keys($result['node']);
      }
      $all_nids = db_query("SELECT nid FROM {taxonomy_index} WHERE tid = :tid", array(':tid' => $tid))->fetchCol();
      $count = count(array_intersect($nids, $all_nids));
    }
    if ($nodecounter == 'all') {
      $count = db_select('taxonomy_index', 'ti')
            ->condition('tid', (int) $tid)
            ->countQuery()
            ->execute()
            ->fetchField();
    }
  }

  if ($ch_count == 1) {
    // Get an array of children terms
    $all = taxonomyblocks_get_all_children($tid);

    // Add the current tid to the array
    $all[] = $tid;

    $nidarray = array();
    foreach ($all as $ctid) {
      // The nodecounter variable is set to get all nodes regardless of content type.
      if ($nodecounter == 'all') {
      $all_nids = db_query("SELECT nid FROM {taxonomy_index} WHERE tid = :tid", array(':tid' => $ctid))->fetchCol();
      $nidarray = array_merge($nidarray, $all_nids);
      }
      // The nodecounter variable is set to filter by a content type.
      else {
      $query = new EntityFieldQuery();
      $query->entityCondition('entity_type', 'node')
        ->entityCondition('bundle', $nodecounter)
        ->propertyCondition('status', 1);

      $result = $query->execute();
      $nids = array();
      if (isset($result['node'])) {
        $nids = array_keys($result['node']);
      }
      $nidarray = array_merge($nidarray, $nids);
      }
    }

    $count = count($nidarray);
  }

  return $count;
}

function taxonomyblocks_term_has_nodes($tid, $nodecounter = 'all') {
  if ($nodecounter != 'all' && $nodecounter != 'none') {
    $query = new EntityFieldQuery();
    $query->entityCondition('entity_type', 'node')
      ->entityCondition('bundle', $nodecounter)
      ->propertyCondition('status', 1);

    $result = $query->execute();
    if (isset($result['node'])) {
      $nids = array_keys($result['node']);
    }
    $all_nids = 'SELECT nid FROM {taxonomy_index} WHERE tid = :tid';
    $all_nids = db_query($all_nids, array(':tid' => $tid))->fetchCol();
    $nids     = array_intersect($nids, $all_nids);
    if ($nids) {
      $nid = array_pop($nids);
    }
  }
  else {
    $nid = 'SELECT nid FROM {taxonomy_index} WHERE tid = :tid';
    $nid = db_query_range($nid, 0, 1, array(':tid' => $tid))->fetchField();
  }
  return $nid;
}

function taxonomyblocks_get_all_children($parent_tid, $prev = NULL) {
  $children = array();
  $level = '';
  $level = taxonomyblocks_get_one_level_children($parent_tid);

  if (!empty($level) && $prev != NULL) {
    $children = array_merge($level, $prev);
  }
  if (!empty($level) && $prev == NULL) {
      $children = $level;
  }
  if (empty($level) && $prev != NULL) {
    $children = $prev;
  }

  if (!empty($level)) {
    foreach ($level as $ctid) {
      $next = array();
      $next = taxonomyblocks_get_all_children($ctid, $children);
      if (!empty($next)) {
        $children = array_merge($next, $children);
      }
    }
  }
  if (is_array($children)) {
    $children = array_unique($children);
  }
  return $children;
}

function taxonomyblocks_get_one_level_children($parent_tid) {
  $children = array();
  
  foreach (taxonomy_get_children($parent_tid) as $term) {
    $children[]   = $term->tid;
  }
  return $children;
}

function taxonomyblocks_taxonomy_get_toplevel($vid) {
  $tree = taxonomy_get_tree($vid);
  
  foreach ($tree as $term) {
    if (isset($term->parents) && $term->parents[0] == 0) {
      $toplevel_array[] = $term->tid;
    }
  }

  return $toplevel_array;
}