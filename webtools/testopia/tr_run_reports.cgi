#!/usr/bin/perl -wT
# -*- Mode: perl; indent-tabs-mode: nil -*-
#
# The contents of this file are subject to the Mozilla Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
#
# The Original Code is the Bugzilla Testopia System.
#
# The Initial Developer of the Original Code is Greg Hendricks.
# Portions created by Greg Hendricks are Copyright (C) 2001
# Greg Hendricks. All Rights Reserved.
#
# Contributor(s): Greg Hendricks <ghendricks@novell.com>

use strict;
use lib ".";

use Bugzilla;
use Bugzilla::Constants;
use Bugzilla::Error;
use Bugzilla::Util;
use Bugzilla::Testopia::Util;
use Bugzilla::Testopia::Constants;
use Bugzilla::Testopia::Report;

use vars qw($template $vars);
my $template = Bugzilla->template;
my $cgi = Bugzilla->cgi;

Bugzilla->login();

my $type = $cgi->param('type') || '';

$cgi->param('current_tab', 'run');
$cgi->param('viewall', 1);
my $report = Bugzilla::Testopia::Report->new('run', 'tr_list_runs.cgi', $cgi);
$vars->{'report'} = $report;

### From Bugzilla report.cgi by Gervase Markham
my $formatparam = $cgi->param('format');
my $report_action = $cgi->param('report_action');
if ($report_action eq "data") {
    # So which template are we using? If action is "wrap", we will be using
    # no format (it gets passed through to be the format of the actual data),
    # and either report.csv.tmpl (CSV), or report.html.tmpl (everything else).
    # report.html.tmpl produces an HTML framework for either tables of HTML
    # data, or images generated by calling report.cgi again with action as
    # "plot".
    $formatparam =~ s/[^a-zA-Z\-]//g;
    trick_taint($formatparam);
    $vars->{'format'} = $formatparam;
    $formatparam = '';
}
elsif ($report_action eq "plot") {
    # If action is "plot", we will be using a format as normal (pie, bar etc.)
    # and a ctype as normal (currently only png.)
    $vars->{'cumulate'} = $cgi->param('cumulate') ? 1 : 0;
    $vars->{'x_labels_vertical'} = $cgi->param('x_labels_vertical') ? 1 : 0;
    $vars->{'data'} = $report->{'image_data'};
}
else {
    ThrowCodeError("unknown_action", {action => $cgi->param('report_action')});
}
 
my $format = $template->get_format("testopia/reports/report", $formatparam,
                               scalar($cgi->param('ctype')));

my $filename = "report-" . $report->{'date'} . ".$format->{extension}";
print $cgi->header(-type => $format->{'ctype'},
                   -content_disposition => "inline; filename=$filename");

$template->process("$format->{'template'}", $vars)
    || ThrowTemplateError($template->error());

exit;

