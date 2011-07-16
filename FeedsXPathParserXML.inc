<?php

/**
 * @files
 * Provides the FeedsXPathParserXML class.
 */
class FeedsXPathParserXML extends FeedsXPathParserBase {

  /**
   * Implements FeedsParser::parse().
   */
  public function parse(FeedsSource $source, FeedsFetcherResult $fetcher_result) {
    parent::parse($source, $fetcher_result);
    $raw = $fetcher_result->getRaw();

    if (empty($raw)) {
      drupal_set_message(t('Feeds XPath parser: The XML document is empty.'), 'warning');
      return;
    }
    if (isset($this->source_config['exp']['tidy']) && $this->source_config['exp']['tidy']) {
      $config = array(
        'input-xml' => TRUE,
        'wrap'      => 0,
        'tidy-mark' => FALSE,
      );
      // Default tidy encoding is UTF8.
      $encoding = $this->source_config['exp']['tidy_encoding'];
      $raw = tidy_repair_string(trim($raw), $config, $encoding);
    }
    $this->doc = new DOMDocument();
    $this->doc->loadXML($raw);
    unset($raw);
    $this->query();
    unset($this->doc);
    return $this->parserResult;
  }
}
