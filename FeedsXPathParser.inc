<?php
// $Id$


define('FEEDS_XPATH_PARSER_HTML', 0);
define('FEEDS_XPATH_PARSER_XML', 1);
define('FEEDS_XPATH_PARSER_REGEX', 2);
define('FEEDS_XPATH_PARSER_QUERYPATH', 3);

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

    $mappings = feeds_importer($this->id)->processor->config['mappings'];
    $source_config = $source->getConfigFor($this);

    if ($source_config['parser_type'] == FEEDS_XPATH_PARSER_XML) {
      require_once 'FeedsXPathParserHTML.inc';
      $this->parser = new FeedsXPathParserXML($source_config, $mappings);
    }

    elseif ($source_config['parser_type'] == FEEDS_XPATH_PARSER_HTML) {
      require_once 'FeedsXPathParserHTML.inc';
      $this->parser = new FeedsXPathParserHTML($source_config, $mappings);
    }

    elseif ($source_config['parser_type'] == FEEDS_XPATH_PARSER_REGEX) {
      require_once 'FeedsXPathParserREGEX.inc';
      $this->parser = new FeedsXPathParserREGEX($source_config, $mappings);
    }

    elseif ($source_config['parser_type'] == FEEDS_XPATH_PARSER_QUERYPATH) {
      require_once 'FeedsXPathParserQueryPath.inc';
      $this->parser = new FeedsXPathParserQueryPath($source_config, $mappings);
    }

    $output = $this->parser->parse($batch->getRaw());

    if ($output) {
      $batch->setItems($output);
    }
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
