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

    if (!empty($this->source_config['exp']['tidy'])) {
      $config = array(
        'input-xml' => TRUE,
        'wrap'      => 0,
        'tidy-mark' => FALSE,
      );
      // Default tidy encoding is UTF8.
      $encoding = $this->source_config['exp']['tidy_encoding'];
      $raw = tidy_repair_string(trim($fetcher_result->getRaw()), $config, $encoding);
    }
    $this->doc = new DOMDocument();
    $use = $this->errorStart();
    $success = $this->doc->loadXML($fetcher_result->getRaw());
    $this->errorStop($use, $this->source_config['exp']['errors']);
    if (!$success) {
      throw new Exception(t('There was an error parsing the XML document.'));
    }
    $this->query();
    unset($this->doc);
    return $this->parserResult;
  }
}
