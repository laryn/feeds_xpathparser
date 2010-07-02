<?php
// $Id$


define('FEEDS_XPATH_PARSER_HTML', 0);
define('FEEDS_XPATH_PARSER_XML', 1);
define('FEEDS_XPATH_PARSER_REGEX', 2);

/**
 * @file
 *
 * Provides the Feeds XPath Parser.
 */

/**
 * Takes a document and runs user provided XPath queries against it.
 */
class FeedsXPathParser extends FeedsParser {

  /**
   * Implementation of FeedsParser::parse().
   */
  public function parse(FeedsImportBatch $batch, FeedsSource $source) {
    $raw = $batch->getRaw();

    $this->config = $source->getConfigFor($this);
    $this->rawXML = array_keys(array_filter($this->config['rawXML']));

    if ($this->config['parser_type'] == FEEDS_XPATH_PARSER_XML) {
      $output = $this->parseXML($raw);
    }
    elseif ($this->config['parser_type'] == FEEDS_XPATH_PARSER_HTML) {
      $output = $this->parseHTML($raw);
    }
    elseif ($this->config['parser_type'] == FEEDS_XPATH_PARSER_REGEX) {
      $output = $this->parseREGEX($raw);
    }

    if ($output) {
      $batch->setItems($output);
    }
  }

  private function parseXML($raw) {
    $dom = new DOMDocument();
    $success = @$dom->loadXML($raw);
    return $this->query($success, $dom);
  }

  private function parseHTML($raw) {
    $dom = new DOMDocument();
    $success = @$dom->loadHTML($raw);
    return $this->query($success, $dom);
  }

  private function query($success, $dom) {
    if (!$success) {
      drupal_set_message(t('There was an error parsing the document.
        Please make sure you have selected the correct parser.'), 'error');
      return;
    }

    $xml = simplexml_import_dom($dom);
    unset($dom);

    $mappings = feeds_importer($this->id)->processor->config['mappings'];

    list($sources, $queries) = $this->getSourcesQueries($mappings);

    $results = array();
    foreach ($queries as $key => $query) {
      $result = @$xml->xpath($query);
      if (!$result) {
        drupal_set_message(t('There was an error with one of your XPath queries.
          Make sure the syntax is valid.'), 'error');
        return;
      }
      $results[$sources[$key]] = $result;
    }
    unset($xml);

    $output = array();
    foreach ($results as $source => $items) {
      if (!isset($count) || $count == count($items)) {
        foreach ($items as $key => $item) {
          if (in_array($source, $this->rawXML)) {
            $item = $item->asXML();
          }
          else {
            $item = (string) $item;
          }
          $output[$key] = array_merge((array)$output[$key], array($source => $item));
        }
        $count = count($items);
      }
      else {
        drupal_set_message(t('Mismatching results.
          Queries must produce the same number of items.'), 'error');
        return;
      }
    }
    return $output;
  }

  private function parseREGEX($raw) {
    $mappings = feeds_importer($this->id)->processor->config['mappings'];

    list($sources, $queries) = $this->getSourcesQueries($mappings);

    $results = array();

    foreach ($queries as $key => $query) {
      $success = @preg_match_all($query, $raw, $matches);

      if ($success !== FALSE) {

        if (isset($matches[1])) {
          array_shift($matches);
          $merged = array();
          foreach ($matches as $match) {
            foreach ($match as $k => $item) {
              $merged[$k][] = $item;
            }
          }
          foreach ($merged as $k => $values) {
            $merged[$k] = implode(' ', $values);
          }
          $results[$sources[$key]] = $merged;
        }

        else {
          $results[$sources[$key]] = $matches[0];
        }
      }
      else {
        drupal_set_message(t('There was an error with one of your regex queries.
          Make sure the syntax is valid.'), 'error');
        return;
      }
    }

    $output = array();
    foreach ($results as $source => $items) {
      if (!isset($count) || $count == count($items)) {
        foreach ($items as $key => $item) {
          $output[$key] = array_merge((array) $output[$key], array($source => $item));
        }
        $count = count($items);
      }
      else {
        drupal_set_message(t('Mismatching results.
          Queries must produce the same number of items.'), 'error');
        return;
      }
    }
    return $output;
  }


  private function getSourcesQueries($mappings) {
    foreach ($mappings as $mapping) {
      $source = $mapping['source'];
      if (trim($this->config[$source])) {
       $sources[] = $source;
       $queries[] = $this->config[$source];
      }
    }
    return array($sources, $queries);
  }

  /**
   * Define defaults.
   */
  public function sourceDefaults() {
    return array('XML' => 0);
  }

  /**
   * Source form.
   */
  public function sourceForm($source_config) {
    $form = array();
    $form['#weight'] = -10;

    $mappings = feeds_importer($this->id)->processor->config['mappings'];
    $sources = $uniques = array();

    foreach ($mappings as $mapping) {
      $sources[] = $mapping['source'];
      if ($mapping['unique']) {
        $uniques[] = $mapping['source'];
      }
    }

    $items = array(
      format_plural(count($uniques),
        t('Field <strong>!column</strong> is mandatory and considered unique: only one item per !column value will be created.',
          array('!column' => implode(', ', $uniques))),
        t('Fields <strong>!columns</strong> are mandatory and values in these columns are considered unique: only one entry per value in one of these columns will be created.',
          array('!columns' => implode(', ', $uniques)))),
    );

    $form['help']['#value'] = '<div class="help">' . theme('item_list', $items) . '</div>';

    ctools_include('dependent');

    $form['parser_type'] = array(
      '#title'    => t('Select the parsing engine to use'),
      '#type'     => 'radios',
      '#options'   => array('HTML', 'XML', 'REGEX'),
      '#default_value' => isset($source_config['parser_type']) ?
                          $source_config['parser_type'] : FEEDS_XPATH_PARSER_HTML,
    );

    $regex_help = array(
      t('The regex query should be in the form <strong>/.*/</strong>.'),
      t('If a capture group is not specified then the fulltext match is returned.'),
      t('If more than one capture group is specified, then they wil be added together with a space in between.'),
    );

    $form['regex_help'] = array(
      '#type' => 'fieldset',
      '#input' => TRUE,
      '#process'       => array('ctools_dependent_process'),
      '#prefix'        => '<div id="edit-feeds-FeedsXPathParser-regex-help-wrapper"><div id="edit-feeds-FeedsXPathParser-regex-help">',
      '#suffix'        => '</div></div>',
      '#dependency'    => array(
        'radio:feeds[FeedsXPathParser][parser_type]' => array(
          FEEDS_XPATH_PARSER_REGEX,
        )
      ),
    );

    $form['regex_help']['text']['#value'] = '<div class="help">' .
                                            theme('item_list', $regex_help) .
                                            '</div>';

    foreach ($sources as $source) {
      $form[$source] = array(
        '#type'          => 'textfield',
        '#title'         => $source,
        '#description'   => t('The query string to run.'),
        '#default_value' => isset($source_config[$source]) ? $source_config[$source] : '',
      );
    }

    $form['rawXML'] = array(
      '#type'          => 'checkboxes',
      '#title'         => t('Select the queries you would like to return raw XML/HTML'),
      '#options'       => array_combine($sources, $sources),
      '#process'       => array('ctools_dependent_process', 'expand_checkboxes'),
      '#prefix'        => '<div id="edit-feeds-FeedsXPathParser-rawXML-wrapper"><div id="edit-feeds-FeedsXPathParser-rawXML">',
      '#suffix'        => '</div></div>',
      '#default_value' => isset($source_config['rawXML']) ? $source_config['rawXML'] : array(),
      '#dependency'    => array(
        'radio:feeds[FeedsXPathParser][parser_type]' => array(
          FEEDS_XPATH_PARSER_HTML,
          FEEDS_XPATH_PARSER_XML,
        )
      ),
    );

    return $form;
  }
}
