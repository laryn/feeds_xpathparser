<?php
// $Id$

function _feeds_xpathparser_parser_types() {
  return array(
    'HTML'      => array(
      'file'  => 'FeedsXPathParserHTML.inc',
      'class' => 'FeedsXPathParserHTML',
    ),
    'XML'       => array(
      'file'  => 'FeedsXPathParserHTML.inc',
      'class' => 'FeedsXPathParserXML',
    ),
    'QueryPath' => array(
      'file'  => 'FeedsXPathParserQueryPath.inc',
      'class' => 'FeedsXPathParserQueryPath',
    ),
    'REGEX' => array(
      'file'  => 'FeedsXPathParserREGEX.inc',
      'class' => 'FeedsXPathParserREGEX',
    )
  );
}

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
    if (empty($mappings)) {
      throw new Exception(t('FeedsXPathParser: No mappings were defined.'));
    }
    $source_config = $source->getConfigFor($this);

    $types = _feeds_xpathparser_parser_types();

    require_once $types[$source_config['parser_type']]['file'];
    $class = $types[$source_config['parser_type']]['class'];
    $parser = new $class($source_config, $mappings);

    $output = $parser->parse($batch->getRaw());

    if ($output) {
      $batch->setItems($output);
    }
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

    if (empty($sources)) {
      $form['message']['#value'] = 'FeedsXPathParser: No mappings were defined.';
      return $form;
    }

    ctools_include('dependent');

    $types = array_keys(_feeds_xpathparser_parser_types());

    if (!module_exists('querypath')) {
      unset($types['QueryPath']);
    }

    $form['parser_type'] = array(
      '#title'    => t('Select the parsing engine to use'),
      '#type'     => 'radios',
      '#options'   => array_combine($types, $types),
      '#default_value' => isset($source_config['parser_type']) ?
                          $source_config['parser_type'] : 'HTML',
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
          'REGEX',
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
        'radio:feeds[FeedsXPathParser][parser_type]' => array('QueryPath')
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
          'HTML', 'XML', 'QueryPath',
        )
      ),
    );

    return $form;
  }
}
