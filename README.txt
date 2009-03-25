Easy SQL generation for Drupal
=====================================

This is a class that allows for easy SQL generation for Drupal. The base class is preconfigured for the node, node_revisions and node_comment_statistics tables, but can easily be subclassed or reconfigured to support any other tables.

The QueryBuilder::query method returns an array with the generated sql as the first element and the parameters for the query as the second.

Quick examples:

$builder = new QueryBuilder();

Call
===========
list($sql, $params) = $builder->query(
  array('nid', 'title', 'comment_count')
);

Result
===========
$sql = "SELECT n.nid AS nid, n.title AS title, cs.comment_count AS comment_count
FROM {node} AS n
	INNER JOIN {node_comment_statistics} AS cs ON cs.nid=n.nid
ORDER BY n.created DESC";
$params = array();

Call
===========
list($sql, $params) = $builder->query(
  array('nid', 'title', 'teaser'), 
  array('created'=>'1236938171:')
);

Result
===========
$sql = "SELECT n.nid AS nid, n.title AS title, r.teaser AS teaser
FROM {node} AS n
	INNER JOIN {node_revisions} AS r ON r.vid=n.vid
WHERE n.created>=%d
ORDER BY n.created DESC";
$params = array(
  1236938171,
);

Call
===========
list($sql, $params) = $builder->query(
  array('nid', 'title'), 
  array('created'=>'1236938171:1237298981', 'type'=>'event')
);

Result
===========
$sql = "SELECT n.nid AS nid, n.title AS title
FROM {node} AS n
WHERE n.created>=%d
	AND n.created<=%d
	AND n.type='%s'
ORDER BY n.created DESC";
$params = array(
  1236938171,
  1237298981,
  'event',
);

Call
===========
list($sql, $params) = $builder->query(
  array('nid', 'title', 'teaser'), 
  array('created'=>'1236938171:', 'sort_field'=>'comment_count', 'sort_order'=>'DESC')
);

Result
===========
$sql = "SELECT n.nid AS nid, n.title AS title, r.teaser AS teaser
FROM {node} AS n
	INNER JOIN {node_revisions} AS r ON r.vid=n.vid
	INNER JOIN {node_comment_statistics} AS cs ON cs.nid=n.nid
WHERE n.created>=%d
ORDER BY cs.comment_count DESC";
$params = array(
  1236938171,
);