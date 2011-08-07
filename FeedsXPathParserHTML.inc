<?php

/**
 * @files
 * Provides the FeedsXPathParserHTML class.
 */
class FeedsXPathParserHTML extends FeedsXPathParserBase {

  /**
   * Implements FeedsParser::parse().
   */
  public function parse(FeedsSource $source, FeedsFetcherResult $fetcher_result) {
    parent::parse($source, $fetcher_result);

    if (!empty($this->source_config['exp']['tidy'])) {
      $config = array(
        'merge-divs'       => FALSE,
        'merge-spans'      => FALSE,
        'join-styles'      => FALSE,
        'drop-empty-paras' => FALSE,
        'wrap'             => 0,
        'tidy-mark'        => FALSE,
        'escape-cdata'     => TRUE,
        'word-2000'        => TRUE,
      );
      // Default tidy encoding is UTF8.
      $encoding = $this->source_config['exp']['tidy_encoding'];
      $raw = tidy_repair_string(trim($fetcher_result->getRaw()), $config, $encoding);
    }
    else {
      $raw = $fetcher_result->getRaw();
    }
    $this->doc = new DOMDocument();
    // Use our own error handling.
    $use = $this->errorStart();
    $success = $this->doc->loadHTML($raw);
    unset($raw);
    $this->errorStop($use, $this->source_config['exp']['errors']);
    if (!$success) {
      throw new Exception(t('There was an error parsing the HTML document.'));
    }
    // Parse!
    $this->query();
    unset($this->doc);
    return $this->parserResult;
  }
}
