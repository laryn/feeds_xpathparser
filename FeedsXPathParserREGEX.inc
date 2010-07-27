<?php
// $Id$

class FeedsXPathParserREGEX {
  public function __construct($config, $mappings) {
    $this->config = $config;
    $this->setSourcesQueries($mappings);

  }

  private function setSourcesQueries($mappings) {
    $this->sources = array();
    $this->queries = array();

    foreach ($mappings as $mapping) {
      $source = $mapping['source'];
      if ($query = trim($this->config['sources'][$source])) {
       $this->sources[] = $source;
       $this->queries[] = $query;
      }
    }
  }

  public function parse($raw) {
    $success = @preg_match_all($this->config['context'], $raw, $matches);

    if ($success === FALSE) {
      throw new Exception(t('There was an error with the regex: %query', array('%query' => $this->config['context'])));
    }

    if (isset($matches[1])) {
      $matches = $matches[1];
    }
    else {
      $matches = $matches[0];
    }
    
    $parsed_items = array();
    foreach ($matches as $item) {
      $parsed_item = array();
      foreach ($this->sources as $source) {
        $parsed_item[$source] = $this->getSourceElement($item, $source);
      }
      $parsed_items[] = $parsed_item;
    }
    return $parsed_items;
  }

  public function getSourceElement($item, $element_key) {
    $query = $this->queries[array_search($element_key, $this->sources)];
    $success = @preg_match_all($query, $item, $matches);

    if ($success !== FALSE) {
      if (isset($matches[1])) {
        array_shift($matches);
        $merged = array();
        foreach ($matches as $match) {
          foreach ($match as $item) {
            $merged[] = $item;
          }
        }
        return implode(' ', $merged);
      }
      else {
        return implode(' ' , $matches[0]);
      }
    }
    else {
      drupal_set_message(t('There was an error with the regex: %query', array('%query' => $query)));
    }
  }
}
