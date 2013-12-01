<?php
//run using "php -d enable_dl=On extension=./kolabcalendaring.so test.php [--verbose]"

include("kolabformat.php");
include("kolabcalendaring.php");

/////// Basic unit test facilities

$errors = 0;
$verbose = preg_match('/\s(--verbose|-v)\b/', join(' ', $_SERVER['argv']));

function assertequal($got, $expect, $name) {
	global $verbose, $errors;

	if ($got == $expect) {
		if ($verbose)
			print "OK - $name\n";
		return true;
	}
	else {
		$errors++;
		print "FAIL - $name\n";
		print "-- Expected " . var_export($expect, true) . ", got " . var_export($got, true) . " --\n";
		return false;
	}
}

function asserttrue($arg, $name) {
	return assertequal($arg, true, $name);
}

function assertfalse($arg, $name) {
	return assertequal($arg, false, $name);
}


/////// Test EventCal recurrence

$xml = <<<EOF
<icalendar xmlns="urn:ietf:params:xml:ns:icalendar-2.0">
  <vcalendar>
    <properties>
      <prodid>
        <text>Libkolab-0.4 Libkolabxml-0.9</text>
      </prodid>
      <version>
        <text>2.0</text>
      </version>
      <x-kolab-version>
        <text>3.0dev1</text>
      </x-kolab-version>
    </properties>
    <components>
      <vevent>
        <properties>
          <uid>
            <text>DDDEBE616DB7480A003725D1D7C4C2FE-8C02E7EEB49870A2</text>
          </uid>
          <created><date-time>2012-10-23T11:04:53Z</date-time></created>
          <dtstamp><date-time>2012-10-23T13:04:53Z</date-time></dtstamp>
          <sequence>
            <integer>0</integer>
          </sequence>
          <class>
            <text>PUBLIC</text>
          </class>
          <dtstart>
            <parameters>
              <tzid>
                <text>/kolab.org/Europe/Paris</text>
              </tzid>
            </parameters>
            <date-time>2012-10-23T14:00:00</date-time>
          </dtstart>
          <dtend>
            <parameters>
              <tzid>
                <text>/kolab.org/Europe/Paris</text>
              </tzid>
            </parameters>
            <date-time>2012-10-23T15:30:00</date-time>
          </dtend>
          <rrule>
            <recur>
              <freq>DAILY</freq>
              <count>4</count>
              <interval>2</interval>
            </recur>
          </rrule>
          <summary>
            <text>Recurring with libkolab</text>
          </summary>
        </properties>
      </vevent>
    </components>
  </vcalendar>
</icalendar>
EOF;

$e = kolabformat::readEvent($xml, false);
$ec = new EventCal($e);

$rstart = new cDateTime(2012,8,1, 0,0,0);
# asserttrue($ec->getNextOccurence($rstart) instanceof cDateTime, "EventCal::getNextOccurence() returning cDateTime instance");

$next = new cDateTime($ec->getNextOccurence($rstart));
assertequal(
	sprintf("%d-%d-%d %02d:%02d:%02d", $next->year(), $next->month(), $next->day(), $next->hour(), $next->minute(), $next->second()),
	"2012-10-23 14:00:00",
	"EventCal first recurrence"
);

$next = new cDateTime($ec->getNextOccurence($next));
assertequal(
	sprintf("%d-%d-%d %02d:%02d:%02d", $next->year(), $next->month(), $next->day(), $next->hour(), $next->minute(), $next->second()),
	"2012-10-25 14:00:00",
	"EventCal second recurrence"
);

$end = new cDateTime($ec->getOccurenceEndDate($next));
assertequal(
	sprintf("%d-%d-%d %02d:%02d:%02d", $end->year(), $end->month(), $end->day(), $end->hour(), $end->minute(), $end->second()),
	"2012-10-25 15:30:00",
	"EventCal::getOccurenceEndDate"
);

$last = new cDateTime($ec->getLastOccurrence());
assertequal(
	sprintf("%d-%d-%d %02d:%02d:%02d", $last->year(), $last->month(), $last->day(), $last->hour(), $last->minute(), $last->second()),
	"2012-10-29 14:00:00",
	"EventCal::getLastOccurence"
);


// terminate with error status
exit($errors);
