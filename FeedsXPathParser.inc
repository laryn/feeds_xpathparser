<?php
// $Id$


define('FEEDS_XPATH_PARSER_HTML', 0);
define('FEEDS_XPATH_PARSER_XML', 1);
define('FEEDS_XPATH_PARSER_REGEX', 2);
define('FEEDS_XPATH_PARSER_QUERYPATH', 3);
define('FEEDS_XPATH_PREG', '/(\/{0,2})(?<!@)(?<!:)(?<!\s)(?<!-)(?<!\')(?<!=)(?<!\[)(?<!")\b(\w+)\b(?!:)(?!\()/i');

/**
 * @file
 *
 * Provides the Feeds XPath Parser.
 */

/**
 * Takes a document and runs user provided XPath or REGEX queries against it.
 */
class FeedsXPathParser extends FeedsParser {

  /**
   * Implementation of FeedsParser::parse().
   */
  public function parse(FeedsImportBatch $batch, FeedsSource $source) {
    if (!function_exists('simplexml_import_dom')) {
      throw new Exception(t('Feeds XML Parser requires SimpleXML PHP extension.'));
    }

    $mappings = feeds_importer($this->id)->processor->config['mappings'];
    $source_config = $source->getConfigFor($this);

    if ($source_config['parser_type'] == FEEDS_XPATH_PARSER_XML) {
      require_once 'FeedsXPathParserHTML.inc';
      $this->parser = new FeedsXPathParserXML;
      $output = $this->parser->parse($batch->getRaw(), $source_config, $mappings);
    }

    elseif ($source_config['parser_type'] == FEEDS_XPATH_PARSER_HTML) {
      require_once 'FeedsXPathParserHTML.inc';
      $this->parser = new FeedsXPathParserHTML;
      $output = $this->parser->parse($batch->getRaw(), $source_config, $mappings);
    }

    elseif ($source_config['parser_type'] == FEEDS_XPATH_PARSER_REGEX) {
      $output = $this->parseREGEX($batch->getRaw());
    }

    elseif ($source_config['parser_type'] == FEEDS_XPATH_PARSER_QUERYPATH && module_exists('querypath')) {
      require_once 'FeedsXPathParserQueryPath.inc';
      $this->parser = new FeedsXPathParserQueryPath;
      $output = $this->parser->parse($batch->getRaw(), $source_config, $mappings);
    }

    if ($output) {
      $batch->setItems($output);
    }
  }

  private function parseREGEX($raw) {
    $mappings = feeds_importer($this->id)->processor->config['mappings'];

    $this->setSourcesQueries($mappings);

    $results = array();

    foreach ($this->queries as $key => $query) {
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
          $results[$this->sources[$key]] = $merged;
        }

        else {
          $results[$this->sources[$key]] = $matches[0];
        }
      }
      else {
        drupal_set_message(t('There was an error with the regex: %query', array('%query' => $query)));
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

  function getSourceElement($item, $element_key) {
    return $this->parser->getSourceElement($item, $element_key);
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

    ctools_include('dependent');

    $supported = array('HTML', 'XML', 'REGEX');

    if (module_exists('querypath')) {
      $supported[] = 'QueryPath';
    }

    $form['parser_type'] = array(
      '#title'    => t('Select the parsing engine to use'),
      '#type'     => 'radios',
      '#options'   => $supported,
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

    $form['context'] = array(
      '#type'          => 'textfield',
      '#title'         => t('Context'),
      '#required'      => TRUE,
      '#description'   => t('This is the base query, all other queries will run in this context.'),
      '#default_value' => isset($source_config['context']) ? $source_config['context'] : '',
    );

    $form['sources'] = array(
      '#type' => 'fieldset',
    );
    $items = array(
      format_plural(count($uniques),
        t('Field <strong>!column</strong> is mandatory and considered unique: only one item per !column value will be created.',
          array('!column' => implode(', ', $uniques))),
        t('Fields <strong>!columns</strong> are mandatory and values in these columns are considered unique: only one entry per value in one of these columns will be created.',
          array('!columns' => implode(', ', $uniques)))),
    );

    $form['sources']['help']['#value'] = '<div class="help">' . theme('item_list', $items) . '</div>';

    $form['attrs'] = array(
      '#type' => 'fieldset',
      '#input' => TRUE,
      '#prefix'        => '<div id="edit-feeds-FeedsXPathParser-attrs-wrapper"><div id="edit-feeds-FeedsXPathParser-attrs">',
      '#suffix'        => '</div></div>',
      '#process'       => array('ctools_dependent_process'),
      '#dependency'    => array(
        'radio:feeds[FeedsXPathParser][parser_type]' => array(
          FEEDS_XPATH_PARSER_QUERYPATH,
        )
      ),
    );

    foreach ($sources as $source) {
      $form['sources'][$source] = array(
        '#type'          => 'textfield',
        '#title'         => $source,
        '#description'   => t('The query string to run.'),
        '#default_value' => isset($source_config['sources'][$source]) ? $source_config['sources'][$source] : '',
      );

      $form['attrs'][$source] = array(
        '#type'          => 'textfield',
        '#title'         => $source . ' Attribute',
        '#description'   => t('The attribute to return.'),
        '#default_value' => isset($source_config['attrs'][$source]) ? $source_config['attrs'][$source] : '',
      );
    }

    $form['rawXML'] = array(
      '#type'          => 'checkboxes',
      '#title'         => t('Select the queries you would like to return raw XML or HTML'),
      '#options'       => array_combine($sources, $sources),
      '#process'       => array('ctools_dependent_process', 'expand_checkboxes'),
      '#prefix'        => '<div id="edit-feeds-FeedsXPathParser-rawXML-wrapper"><div id="edit-feeds-FeedsXPathParser-rawXML">',
      '#suffix'        => '</div></div>',
      '#default_value' => isset($source_config['rawXML']) ? $source_config['rawXML'] : array(),
      '#dependency'    => array(
        'radio:feeds[FeedsXPathParser][parser_type]' => array(
          FEEDS_XPATH_PARSER_HTML,
          FEEDS_XPATH_PARSER_XML,
          FEEDS_XPATH_PARSER_QUERYPATH,
        )
      ),
    );

    return $form;
  }
}
