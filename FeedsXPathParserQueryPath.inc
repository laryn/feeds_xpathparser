<?php
// $Id$

class FeedsXPathParserQueryPath {

  public function __construct($config, $mappings) {
    if (!module_exists('querypath')) {
      throw new Exception(t('FeedsXPathParser requires the QueryPath module to parse QueryPath.'));
    }
    $this->config = $config;
    $this->setSourcesQueries($mappings);
  }

  public function parse($raw, $config) {

    $output = array();
    foreach (qp($raw, $config['context']) as $child) {
      $output[] = $child->html();
    }
    return $output;
  }

  public function getSourceElement($item, $element_key) {
    $rawXML = array_keys(array_filter($this->config['rawXML']));
    $key   = array_search($element_key, $this->sources);
    $query = $this->queries[$key];
    $attr  = $this->attrs[$key];

    if ($query != '') {
      $qp = qp($item, $query);
    }
    else {
      $qp = qp($item);
    }

    if ($attr != '') {
      return $qp->attr($attr);
    }

    if (in_array($element_key, $rawXML)) {
      return $qp->html();
    }
    else {
      return $qp->text();
    }
  }

  private function setSourcesQueries($mappings) {
    $this->sources = array();
    $this->queries = array();
    $this->attrs   = array();

    foreach ($mappings as $mapping) {
      $source = $mapping['source'];
      $this->sources[] = $source;
      $this->queries[] = trim($this->config['sources'][$source]);
      $this->attrs[]   = trim($this->config['attrs'][$source]);
    }
  }
}
