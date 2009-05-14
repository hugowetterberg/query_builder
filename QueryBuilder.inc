<?php
// $Id$

class QueryBuilder {
  const CONDITION_NONE = 0;
  const CONDITION_MATCH = 1;
  //const CONDITION_CONTAINS = 2;
  const CONDITION_RANGE = 4;

  private $tables = array();
  private $fields = array();

  public $defaultOrderField = 'created';
  public $defaultFields = array();
  public $rewriteSql = TRUE;

  public function __construct($default_setup=TRUE) {
    if ($default_setup) {
      $this->defaultFields = array('nid', 'created', 'title', 'teaser');

      $this->add_table('n', '{node} AS n', NULL, array(
        'nid' => array(
          'type' => 'int',
          'condition' => self::CONDITION_RANGE,
          'sortable' => TRUE,
        ),
        'type' => array(
          'condition' => self::CONDITION_MATCH,
        ),
        'uid' => array(
          'type' => 'int',
          'condition' => self::CONDITION_RANGE,
          'sortable' => TRUE,
        ),
        'created' => array(
          'type' => 'int',
          'condition' => self::CONDITION_RANGE,
          'sortable' => TRUE,
        ),
        'changed' => array(
          'type' => 'int',
          'condition' => self::CONDITION_RANGE,
          'sortable' => TRUE,
        ),
        'sticky' => array(
          'type' => 'int',
          'condition' => self::CONDITION_MATCH,
          'sortable' => TRUE,
        ),
        'status' => array(
          'type' => 'int',
          'condition' => self::CONDITION_MATCH,
          'sortable' => TRUE,
        ),
        'promote' => array(
          'type' => 'int',
          'condition' => self::CONDITION_MATCH,
          'sortable' => TRUE,
        ),
        'title' => array(
          'condition' => self::CONDITION_NONE,
          'sortable' => TRUE,
        ),
      ));
      $this->add_table('r', 'INNER JOIN {node_revisions} AS r ON r.vid=n.vid', 'n', array(
        'body' => array(),
        'teaser' => array(),
        'log' => array(),
      ));
      $this->add_table('cs', 'INNER JOIN {node_comment_statistics} AS cs ON cs.nid=n.nid', 'n', array(
        'last_comment_timestamp' => array(
          'type' => 'int',
          'condition' => self::CONDITION_RANGE,
          'sortable' => TRUE,
        ),
        'last_comment_name' => array(),
        'last_comment_uid' => array(
          'type' => 'int',
          'condition' => self::CONDITION_MATCH,
          'sortable' => TRUE,
        ),
        'comment_count' => array(
          'type' => 'int',
          'condition' => self::CONDITION_RANGE,
          'sortable' => TRUE,
        ),
      ));
    }
  }

  public function add_table($name, $join, $joins_with, $fields=NULL) {
    $this->tables[$name] = array(
      'join' => $join,
      'parent' => $joins_with,
    );
    if (is_array($fields)) {
      foreach ($fields as $field_name => $field) {
        $field['table'] = $name;
        $this->add_field($field_name, $field);
      }
    }
  }

  public function add_field($name, $field) {
    $field = array_merge(array(
      'type' => 'string',
      'condition' => self::CONDITION_NONE,
      'sortable' => FALSE,
      'column' => $name,
    ), $field);

    $this->fields[$name] = $field;
  }

  private function use_field(&$q, $field_name) {
    if (isset($this->fields[$field_name])) {
      $field = $this->fields[$field_name];
      // Make sure that the table that the field belongs to is joined in
      if ($this->ensure_join($q, $field)) {
        return $field;
      }
    }
    return FALSE;
  }

  private function ensure_join(&$q, $field) {
    if (!$q->joined[$field['table']]) {
      return $this->join_table($q, $field['table']);
    }
    return TRUE;
  }

  private function field_column($field) {
    if ($field['expression'])
      return $field['expression'];
    else
      return $field['table'] . '.' . $field['column'];
  }

  private function join_table(&$q, $name) {
    if (isset($this->tables[$name])) {
      $table = $this->tables[$name];
      // Join in parent dependecies first
      if (!empty($table['parent'])) {
        if (!$this->join_table($q, $table['parent'])) {
          return FALSE;
        }
      }
      $q->joined[$name] = $table['join'];
      return TRUE;
    }
    return FALSE;
  }

  public function describe() {
    $desc = array();

    foreach ($this->fields as $name => $def) {
      $fdesc = $def;
      unset($fdesc['column']);
      unset($fdesc['table']);
      unset($fdesc['expression']);
      switch ($fdesc['condition']) {
        case self::CONDITION_RANGE:
          $cond = 'range';
          break;
        case self::CONDITION_MATCH:
          $cond = 'match';
          break;
        default:
          $cond = 'none';
      }
      $fdesc['condition'] = $cond;
      $desc[$name] = $fdesc;
    }
    return $desc;
  }

  public function handle_parameter(&$q, $key, $value) {
    // Add term joins
    if ($key=='terms') {
      $terms = preg_split('/,\s?/', $value);
      $added_terms = array();
      foreach ($terms as $term) {
        if (is_numeric($term) && !isset($added_terms[$term])) {
          $table_name = 'tn_' . $term;
          $q->joined[$table_name] = sprintf('INNER JOIN {term_node} AS tn_%d ON (tn_%d.vid = n.vid AND tn_%d.tid=%d)',
            $term, $term, $term, $term);
          $added_terms[$term] = TRUE;
        }
      }
      return TRUE;
    }
  }

  public function debug_query($field_names=array(), $parameters=array()) {
    global $db_prefix;
    list($sql, $params) = $this->query($field_names, $parameters);
    $sql = preg_replace('/\{([^\}]+)\}/', $db_prefix . '$1', $sql);
    $args = $params;
    array_unshift($args, $sql);
    return call_user_func_array('sprintf', $args);
  }

  public function query($field_names=array(), $parameters=array()) {
    if (empty($field_names)) {
      $field_names = $this->defaultFields;
    }

    if ($this->rewriteSql) {
      // Make sure that the nid is the first column
      // when we use rewrite sql
      $nid_pos = array_search('nid', $field_names);
      if ($nid_pos!==0) {
        array_unshift($field_names, 'nid');
      }
    }

    $q = (object)array(
      'joined' => array(),
      'fields' => array(),
      'conditions' => array(),
      'params' => array(),
      'sort_field' => $this->defaultOrderField,
      'sort_order' => 'DESC',
    );

    // Add the requested fields
    foreach ($field_names as $field_name) {
      // Ignore the field if it isn't known
      if ($field = $this->use_field($q, $field_name)) {
        $q->fields[] = $this->field_column($field) . ' AS ' . $field_name;
      }
    }

    // Interpret the general parameters
    foreach ($parameters as $key => $value) {
      // Check if there's custom handling for the parameter
      if ($this->handle_parameter($q, $key, $value)) {
        continue;
      }
      // Change sorting field
      else if ($key=='sort_field') {
        if ($this->use_field($q, $value)) {
          $q->sort_field = $value;
        }
      }
      // Change sorting order
      else if ($key=='sort_order') {
        $q->sort_order = $value=='ASC' ? 'ASC' : 'DESC';
      }
      // Add field conditions
      else if (isset($this->fields[$key])) {
        if ($field = $this->use_field($q, $key)) {
          // Determine what placeholder we should use
          switch ($field['type']) {
            case 'int':
              $placeholder = '%d';
              break;
            case 'float':
              $placeholder = '%f';
              break;
            default:
              $placeholder = "'%s'";
              break;
          }

          // If this is a range condition
          if (($field['condition'] == self::CONDITION_RANGE && ($cpos = strpos($value, ':'))!==FALSE)) {
            $interval = preg_split('/:/', $value);
            if (count($interval)>1) {
              $col = $this->field_column($field);
              if (strlen($interval[0])) {
                $q->conditions[] = $col . '>=' . $placeholder;
                $q->params[] = $interval[0];
              }
              if (strlen($interval[1])) {
                $q->conditions[] = $col . '<=' . $placeholder;
                $q->params[] = $interval[1];
              }
            }
          }
          // ...if not we treat it as a match condition
          else {
            $col = $this->field_column($field);
            $q->conditions[] = $col . '=' . $placeholder;
            $q->params[] = $value;
          }
        }
      }
    }

    // Get info for the order field
    $order_column = $this->use_field($q->joined, $q->sort_field);

    // Construct the sql query
    $sql = 'SELECT ' . join($q->fields, ', ') .
      "\nFROM " . join($q->joined, "\n\t") .
      (count($q->conditions) ? "\nWHERE " . join($q->conditions, "\n\tAND ") : '');
    if ($order_column) {
      $sql .= "\nORDER BY " . $this->field_column($order_column) . ' ' . $q->sort_order;
    }
    if ($this->rewriteSql) {
      return array(db_rewrite_sql($sql), $q->params);
    }
    else {
      return array($sql, $q->params);
    }
  }
}