---
layout: docs
prev_section: regressiontestsystem
next_section: contact
title: Command Line Parameters
permalink: /docs/commandlineparameters/
published: true
---

Command line parameters are prefixed using a double dash. This convention is used on all platforms to make it possible to reuse scripts across different platforms.

See GNU Standards for [Command Line Interfaces](http://www.gnu.org/prep/standards/html_node/Command_002dLine-Interfaces.html#Command_002dLine-Interfaces).

Examples on how command line options are used are given in 
[Batch Commands]({{ site.baseurl }}/docs/batchcommands)

| Parameter | Description |
|-----------|-------------|
| &#45;&#45;last                   | Open last used project. |
| &#45;&#45;project &lt;filename&gt;     | Open project file &lt;filename&gt;. |
| &#45;&#45;case &lt;casename&gt;        | Import Eclipse case &lt;casename&gt; (do not include .GRID/.EGRID) |
| &#45;&#45;startdir &lt;folder&gt;      | Set startup directory. |
| &#45;&#45;savesnapshots          | Save snapshot of all views to **snapshots** folder in project file folder. Application closes after snapshots have been written. |
| &#45;&#45;size &lt;width&gt; &lt;height&gt;  | Set size of the main application window. |
| &#45;&#45;replaceCase [&lt;caseId&gt;] &lt;newGridFile&gt;  | Replace grid in &lt;caseId&gt; or first case with &lt;newGridFile&gt;. |
| &#45;&#45;replaceSourceCases [&lt;caseGroupId&gt;] &lt;gridListFile&gt; | Replace source cases in &lt;caseGroupId&gt; or first grid case group with the grid files listed in the &lt;gridListFile&gt; file. |
| &#45;&#45;multiCaseSnapshots &lt;gridListFile&gt; | For each grid file listed in the &lt;gridListFile&gt; file, replace the first case in the project and save snapshot of all views. |
| &#45;&#45;help, &#45;&#45;?       | Displays help text and version info |
| &#45;&#45;regressiontest &lt;folder&gt; | Run a regression test on all sub-folders starting with `TestCase*` of the given folder. **RegressionTest.rip** files in the sub-folders will be opened and snapshots of all the views is written to the sub-sub-folder **RegTestGeneratedImages**. Then difference images is generated in the sub-sub-folder **RegTestDiffImages** based on the images in sub-sub-folder **RegTestBaseImages**. The results are presented in **ResInsightRegressionTestReport.html** that is written in the given folder. |
| &#45;&#45;updateregressiontestbase &lt;folder&gt; | For all sub-folders starting with `TestCase*`, copy the images in the sub-sub-folder **RegTestGeneratedImages** to the sub-sub-folder **RegTestBaseImages** after deleting **RegTestBaseImages** completely. |

See also the [Regression Test System ]({{site.baseurl }}/docs/regressiontestsystem) for a more in-depth explanation.