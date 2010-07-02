<?php

class FeedsXPathParserHTML {

  public function parse($raw, $config, $mappings) {
    $this->config = $config;
    $this->setSourcesQueries($mappings);

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

  private function query($xml) {

    $xml = $this->setNamespaces($xml);

    $results = @$xml->xpath($this->config['context']);
    if ($results === FALSE) {
      throw new Exception(t('There was an error with the XPath query: %query',
                            array('%query' => $this->sourceConfig['context'])));
    }
    unset($xml);

    foreach ($results as &$result) {
      $result = $result->asXML();
    }
    return $results;
  }

  private function setNamespaces($xml) {
    $this->namespaces = $xml->getNamespaces(TRUE);

    foreach ($this->namespaces as $prefix => $namespace) {
      if ($prefix === '') {
        $xml->registerXPathNamespace('__default__', $namespace);
        $this->sourceConfig['context'] = preg_replace(FEEDS_XPATH_PREG,
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

    if (is_array($results) && !empty($results)) {
      foreach ($results as &$result) {
        if (in_array($element_key, $rawXML)) {
          $result = $result->asXML();
        }
        else {
          $result = (string)$result;
        }
      }
      return implode(' ', $results);
    }
  }
}

class FeedsXPathParserXML extends FeedsXPathParserHTML {

  public function parse($raw, $config, $mappings) {
    $this->config = $config;
    $this->setSourcesQueries($mappings);

    $xml = new SimpleXMLElement($raw);
    return $this->query($xml);
  }
}
