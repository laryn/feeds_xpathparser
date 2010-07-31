<?php
// $Id$

define('FEEDS_XPATH_PREG', '/(\/{0,2})(?<![=:\'"-\s[])\b(\w+)\b(?!:)(?![\(\s])/i');

class FeedsXPathParserHTML {

  public function __construct($config, $mappings) {
    $this->config = $config;
    $this->setSourcesQueries($mappings);
    if (!extension_loaded('simplexml')) {
      throw new Exception(t('FeedsXPathParser requires SimpleXML to parse HTML or XML.'));
    }
  }

  public function parse($raw) {
    $dom = new DOMDocument();
    $success = @$dom->loadHTML($raw);

    if (!$success) {
      throw new Exception(t('There was an error parsing the HTML document.'));
    }

    $xml = simplexml_import_dom($dom);
    unset($dom);
    return $this->query($xml);
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

  public function query($xml) {

    $xml = $this->setNamespaces($xml);

    $all_items = @$xml->xpath($this->config['context']);
    if ($all_items === FALSE) {
      throw new Exception(t('There was an error with the XPath query: %query',
                            array('%query' => $this->sourceConfig['context'])));
    }
    unset($xml);

    $parsed_items = array();
    foreach ($all_items as $item) {
      $item = $item->asXML();
      $parsed_item = array();
      foreach ($this->sources as $source) {
        $parsed_item[$source] = $this->getSourceElement($item, $source);
      }
      $parsed_items[] = $parsed_item;
    }
    return $parsed_items;
  }

  private function setNamespaces($xml) {
    $this->namespaces = $xml->getNamespaces(TRUE);

    foreach ($this->namespaces as $prefix => $namespace) {
      if ($prefix === '') {
        $xml->registerXPathNamespace('__default__', $namespace);
        $this->config['context'] = preg_replace(FEEDS_XPATH_PREG,
          '$1__default__:$2', $this->config['context']
        );
      }
      else {
        $xml->registerXPathNamespace($prefix, $namespace);
      }
    }
    return $xml;
  }

  public function getSourceElement($item, $element_key) {
    $xml = @new SimpleXMLElement($item);
    $query = $this->queries[array_search($element_key, $this->sources)];
    $results = $xml->xpath($query);
    unset($xml);
    $rawXML = array_keys(array_filter($this->config['rawXML']));

    if (is_array($results)) {
      foreach ($results as &$result) {
        if (in_array($element_key, $rawXML)) {
          $result = $result->asXML();
        }
        else {
          $result = (string)$result;
        }
      }
      if (count($results) == 1) {
        return $results[0];
      }
      elseif (count($results) > 1) {
        return $results;
      }
      else {
        return '';
      }
    }
  }
}

class FeedsXPathParserXML extends FeedsXPathParserHTML {

  public function parse($raw) {

    $xml = new SimpleXMLElement($raw);
    return $this->query($xml);
  }
}
