function scwShowSubmit(el1,el2)
{
		scwNextAction = scwSubmit;
		scwShow(el1,el2);
}
function scwSubmit()
{
	scwNextAction = null;
	document.mainform.submit();
}

// *****************************************************************************
//      Simple Calendar Widget - Cross-Browser Javascript pop-up calendar.
//
//   Copyright (C) 2005-2006  Anthony Garrett
//
//   This library is free software; you can redistribute it and/or
//   modify it under the terms of the GNU Lesser General Public
//   License as published by the Free Software Foundation; either
//   version 2.1 of the License, or (at your option) any later version.
//
//   This library is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//   Lesser General Public License for more details.
//
//   You should have received a copy of the GNU Lesser General Public
//   License along with this library; if not, it is available at
//   the GNU web site (http://www.gnu.org/) or by writing to the
//   Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
//   Boston, MA  02110-1301  USA
//
// *****************************************************************************
//
// Contact:   Sorry, I can't offer support for this but if you find a problem
//            (or just want to tell me how useful you find it), please send
//            me an email at scwfeedback@tarrget.info (Note the two Rs in
//            tarrget).  I will try to fix problems quickly but this is a
//            spare time thing for me.
//
// Credits:   I wrote this from scratch myself but I couldn't have done it
//            without the superb "JavaScript The Definitive Guide" by David
//            Flanagan (Pub. O'Reilly ISBN 0-596-00048-0).  I also recognise
//            a contribution from my experience with PopCalendar 4.1 by
//            Liming(Victor) Weng.
//
// Link back: Please give me credit and link back to my page.  To ensure that
//            search engines give my page a higher ranking you can add the
//            following HTML to any indexed page on your web site:
//
//            <A HREF="http://www.tarrget.info/calendar/scw.html">
//              Simple Calendar Widget by Anthony Garrett
//            </A>
//

// ************************************
// Start of Simple Calendar Widget Code
// ************************************
    var scwDateNow = new Date(Date.parse(new Date().toDateString()));

//******************************************************************************
//------------------------------------------------------------------------------
// Customisation section
//------------------------------------------------------------------------------
//******************************************************************************
    var scwBaseYear        = scwDateNow.getFullYear()-10;
    var scwDropDownYears   = 20;
    var scwLanguage;
	
    function scwSetLanguage()
        {switch (scwLanguage)
            {case 'br':
                // English
                scwToday               = 'Today:';
                scwDrag                = 'click here to drag';
                scwArrMonthNames       = ['Jan','Feb','Mar','Apr','May','Jun',
                                          'Jul','Aug','Sep','Oct','Nov','Dec'];
                scwArrWeekInits        = ['S','M','T','W','T','F','S'];
                scwInvalidDateMsg      = 'The entered date is invalid.\n';
                scwOutOfRangeMsg       = 'The entered date is out of range.';
                scwDoesNotExistMsg     = 'The entered date does not exist.';
                scwInvalidAlert        = ['Invalid date (',') ignored.'];
                scwDateDisablingError  = ['Error ',' is not a Date object.'];
                scwRangeDisablingError = ['Error ',' should consist of two elements.'];
                break;

             default:
                // Francais
                scwToday               = 'Aujourd\'hui:';
                scwDrag                = 'Déplacer le calendrier';
                scwArrMonthNames       = ['Jan','Fév','Mar','Avr','Mai','Juin',
                                          'Jui','Aou','Sep','Oct','Nov','Déc'];
                scwArrWeekInits        = ['Di','Lu','Ma','Me','Je','Ve','Sa'];
                scwInvalidDateMsg      = 'Date invalide\n';
                scwOutOfRangeMsg       = 'Date en dehors de la plage autorisée';
                scwDoesNotExistMsg     = 'La date n\'existe pas';
                scwInvalidAlert        = ['La date (',') n\'est pas reconnue (ignorée)'];
                scwDateDisablingError  = ['Erreur ',' n\'est pas un objet Date'];
                scwRangeDisablingError = ['Erreur ',' doit avoir deux éléments'];
            }
        }

    var scwWeekStart       =    1;
    var scwWeekNumberDisplay    = false;
    var scwWeekNumberBaseDay    = 4;
    var scwArrDelimiters   = ['/','-','.',',',' ',':'];
    var scwDateDisplayFormat = 'DD/MM/YYYY';
    var scwDateOutputFormat  = scwDateDisplayFormat
    var scwDateInputSequence = 'DMY';
    var scwZindex          = 1;
    var scwBlnStrict       = true;
    var scwEnabledDay      = [true, true, true, true, true, true, true,
                              true, true, true, true, true, true, true,
                              true, true, true, true, true, true, true,
                              true, true, true, true, true, true, true,
                              true, true, true, true, true, true, true,
                              true, true, true, true, true, true, true];
    var scwDisabledDates   = new Array();
    var scwActiveToday = true;
	var scwSelHour = "", scwSelMin = "";
    var scwOutOfRangeDisable = true;
    var scwAllowDrag = false;
    var scwClickToHide = false;
    var scwBackground           = '#F0F4F8';    // Calendar background
    var scwHeadText             = '#888888';    // Colour of week headings
    var scwTodayText            = '#888888',
        scwTodayHighlight       = '#CC4444';
    var scwHighlightText        = '#000000',
        scwHighlightBackground  = '#FFFF00';
    var scwDragText             = '#CCCCFF',
        scwDragBackground       = '#9999CC';
    var scwWeekNumberText       = '#CCCCCC',
        scwWeekNumberBackground = '#776677';
    var scwWeekendText          = '#CC6666',
        scwWeekendBackground    = '#CCCCCC';
    var scwExMonthText          = '#666666',
        scwExMonthBackground    = '#CCCCCC';
    var scwCellText             = '#000000',
        scwCellBackground       = '#CCCCCC';
    var scwInDateText           = '#FF0000',
        scwInDateBackground     = '#FFCCCC';
    var scwDisabledWeekendText          = '#CC6666',
        scwDisabledWeekendBackground    = '#999999';
    var scwDisabledExMonthText          = '#666666',
        scwDisabledExMonthBackground    = '#999999';
    var scwDisabledCellText             = '#000000',
        scwDisabledCellBackground       = '#999999';
    var scwDisabledInDateText           = '#FF0000',
        scwDisabledInDateBackground     = '#CC9999';
    document.writeln("<style>");
    document.writeln(   '.scw       {padding:1px;vertical-align:middle;}');
    document.writeln(   'iframe.scw {position:absolute;z-index:' + scwZindex   +
                                    ';top:0px;left:0px;visibility:hidden;'     +
                                    'width:1px;height:1px;}');
    document.writeln(   'table.scw  {padding:0px;visibility:hidden;'           +
                                    'position:absolute;width:200px;'           +
                                    'top:0px;left:0px;z-index:' +(scwZindex+1) +
                                    ';text-align:center;cursor:default;'       +
                                    'padding:1px;vertical-align:middle;'       +
                                    'background-color:' + scwBackground        +
                                    ';border:1px solid grey;border-radius:8px;font-size:10pt;'        +
                                    'font-family:Arial,Helvetica,Sans-Serif;'  +
                                    'font-weight:bold;}');
    document.writeln(   'td.scwDrag {text-align:center;font-size:8pt;' +
                                    'background-color:'  + scwDragBackground +
                                    ';padding:0px 0px;color:' + scwDragText  +
                                    "}");
    document.writeln(   'td.scwHead {padding:0px 0px;text-align:center;}');
    document.writeln(   'select.scwHead {margin:3px 1px;}');
    document.writeln(   'input.scwHead  {height:22px;width:32px;'              +
                                        'vertical-align:middle;'               +
                                        'text-align:center;'    +
                                        'font-size:10pt;font-family:fixedSys;' +
                                        'font-weight:bold;}');
    document.writeln(   'td.scwWeekNumberHead '                                +
                                        '{text-align:center;font-weight:bold;' +
                                        'padding:0px;color:'                   +
                                            scwBackground + ';}');
    document.writeln(   'td.scwWeek     {text-align:center;font-weight:bold;'  +
                                        'padding:0px;color:'                   +
                                            scwHeadText + ';}');
    document.writeln(   'table.scwCells {text-align:right;font-size:8pt;'      +
                                        'width:96%;font-family:'               +
                                        'Arial,Helvetica,Sans-Serif;}');
    document.writeln(   'td.scwCells    {padding:3px;vertical-align:middle;'   +
                                        'text-align:center; width:16px;height:16px;'              +
                                        'font-weight:bold;color:'              +
                                            scwCellText                        +
                                        ';background-color:'                   +
                                            scwCellBackground                  +
                                        '}');
    document.writeln(   'td.scwTime    {padding:3px;vertical-align:middle;'   +
                                        'font-weight:bold; text-align:center; width:16px;color:'              +
                                            scwHeadText                        +
                                        '}');
    document.writeln(   'td.scwTimeCell {padding:3px;vertical-align:middle;'   +
                                        'font-weight:bold;text-align:center; width:16px; color:'              +
                                            scwCellText                        +
                                        ';background-color:'                   +
                                            scwCellBackground                  +
                                        '}');
    document.writeln(   'td.scwWeekNo   {padding:3px;vertical-align:middle;'   +
                                        'width:16px;height:16px;'              +
                                        'font-weight:bold;color:'              +
                                            scwWeekNumberText                  +
                                        ';background-color:'                   +
                                            scwWeekNumberBackground            +
                                        '}');
    document.writeln(   'td.scwWeeks {padding:3px;vertical-align:middle;'      +
                                     'width:16px;height:16px;'                 +
                                     'font-weight:bold;color:' + scwCellText   +
                                     ';background-color:' + scwCellBackground  +
                                     '}');
    document.writeln(   'td.scwFoot  {padding:0px;text-align:center;'          +
                                     'font-weight:normal;color:'               +
                                      scwTodayText + ';}');
    document.writeln("</style>");

//******************************************************************************
//------------------------------------------------------------------------------
// End of customisation section
//------------------------------------------------------------------------------
//******************************************************************************

//  Variables required by both scwShow and scwShowMonth

    var scwTargetEle,
        scwTriggerEle,
        scwMonthSum            = 0,
        scwBlnFullInputDate    = false,
        scwPassEnabledDay      = new Array(),
        scwSeedDate            = new Date(),
        scwParmActiveToday     = true,
        scwWeekStart           = scwWeekStart%7,
        scwToday,
        scwDrag,
        scwArrMonthNames,
        scwArrWeekInits,
        scwInvalidDateMsg,
        scwOutOfRangeMsg,
        scwDoesNotExistMsg,
        scwInvalidAlert,
        scwDateDisablingError,
        scwRangeDisablingError;

    // Add a method to format a date into the required pattern

    Date.prototype.scwFormat =
        function(scwFormat)
            {var charCount = 0,
                 codeChar  = '',
                 result    = '';

             for (var i=0;i<=scwFormat.length;i++)
                {if (i<scwFormat.length && scwFormat.charAt(i)==codeChar)
                        {// If we haven't hit the end of the string and
                         // the format string character is the same as
                         // the previous one, just clock up one to the
                         // length of the current element definition
                         charCount++;
                        }
                 else   {switch (codeChar)
                            {case 'y': case 'Y':
                                result += (this.getFullYear()%Math.
                                            pow(10,charCount)).toString().
                                            scwPadLeft(charCount);
                                break;
                             case 'm': case 'M':
                                // If we find an M, check the number of them to
                                // determine whether to get the month number or
                                // the month name.
                                result += (charCount<3)
                                            ?(this.getMonth()+1).
                                                toString().scwPadLeft(charCount)
                                            :scwArrMonthNames[this.getMonth()];
                                break;
                             case 'd': case 'D':
                                // If we find a D, get the date and format it
                                result += this.getDate().toString().
                                            scwPadLeft(charCount);
                                break;
                             default:
                                // Copy any unrecognised characters across
                                while (charCount-- > 0) {result += codeChar;}
                            }

                         if (i<scwFormat.length)
                            {// Store the character we have just worked on
                             codeChar  = scwFormat.charAt(i);
                             charCount = 1;
                            }
                        }
                }
             return result;
            }

    // Add a method to left pad zeroes

    String.prototype.scwPadLeft =
        function(padToLength)
            {var result = '';
             for (var i=0;i<(padToLength - this.length);i++) {result += '0';}
             return (result + this);
            }

    // Set up a closure so that any next function can be triggered
    // after the calendar has been closed AND that function can take
    // arguments.

    Function.prototype.runsAfterSCW =
        function()  {var func = this,
                         args = new Array(arguments.length);

                     for (var i=0;i<args.length;++i)
                        {args[i] = arguments[i];}

                     return function()
                        {// concat/join the two argument arrays
                         for (var i=0;i<arguments.length;++i)
                            {args[args.length] = arguments[i];}

                         return (args.shift()==scwTriggerEle)
                                    ?func.apply(this, args):null;
                        }
                    };

    // Use a global variable for the return value from the next action
    // IE fails to pass the function through if the target element is in
    // a form and scwNextAction is not defined.

    var scwNextActionReturn, scwNextAction;

// ****************************************************************************
// Start of Function Library
//
//  Exposed functions:
//
//      scwShow             Entry point for display of calendar,
//                              called in main page.
//      showCal             Legacy name of scwShow:
//                              Passes only legacy arguments,
//                              not the optional day disabling arguments.
//
//      scwShowMonth        Displays a month on the calendar,
//                              Called when a month is set or changed.
//
//      scwBeginDrag        Controls calendar dragging.
//
//      scwCancel           Called when the calendar background is clicked:
//                              Calls scwStopPropagation and may call scwHide.
//      scwHide             Hides the calendar, called on various events.
//      scwStopPropagation  Stops the propagation of an event.
//
// ****************************************************************************
		
    function showCal(scwEle,scwSourceEle)    {scwShow(scwEle,scwSourceEle);}
    function scwShow(scwEle,scwSourceEle)
        {scwTriggerEle = scwSourceEle;

         // Take any parameters that there might be from the third onwards as
         // day numbers to be disabled 0 = Sunday through to 6 = Saturday.

         scwParmActiveToday = true;

         for (var i=0;i<7;i++)
            {scwPassEnabledDay[(i+7-scwWeekStart)%7] = true;
             for (var j=2;j<arguments.length;j++)
                {if (arguments[j]==i)
                    {scwPassEnabledDay[(i+7-scwWeekStart)%7] = false;
                     if (scwDateNow.getDay()==i) scwParmActiveToday = false;
                    }
                }
            }

         //   If no value is preset then the seed date is
         //      Today (when today is in range) OR
         //      The middle of the date range.

         scwSeedDate = scwDateNow;

         // Strip space characters from start and end of date input
         scwEle.value = scwEle.value.replace(/^\s+/,'').replace(/\s+$/,'');

         // Set the language-dependent elements

         scwSetLanguage();

         document.getElementById('scwDragText').innerHTML = scwDrag;

         document.getElementById('scwMonths').options.length = 0;
         for (i=0;i<scwArrMonthNames.length;i++)
            document.getElementById('scwMonths').options[i] =
                new Option(scwArrMonthNames[i],scwArrMonthNames[i]);

         document.getElementById('scwYears').options.length = 0;
         for (i=0;i<scwDropDownYears;i++)
            document.getElementById('scwYears').options[i] =
                new Option((scwBaseYear+i),(scwBaseYear+i));

         for (i=0;i<scwArrWeekInits.length;i++)
            document.getElementById('scwWeekInit' + i).innerHTML =
                          scwArrWeekInits[(i+scwWeekStart)%
                                            scwArrWeekInits.length];

         if (document.getElementById('scwFoot'))
            document.getElementById('scwFoot').innerHTML =
                    scwToday + " " +
                    scwDateNow.scwFormat(scwDateDisplayFormat);

         if (scwEle.value.length==0)
            {// If no value is entered and today is within the range,
             // use today's date, otherwise use the middle of the valid range.

             scwBlnFullInputDate=false;

             if ((new Date(scwBaseYear+scwDropDownYears-1,11,31))<scwSeedDate ||
                 (new Date(scwBaseYear,0,1))                     >scwSeedDate
                )
                {scwSeedDate = new Date(scwBaseYear +
                                        Math.floor(scwDropDownYears / 2), 5, 1);
                }
            }
         else
            {function scwInputFormat(scwEleValue)
                {var scwArrSeed = new Array(),
                     scwArrInput = scwEle.value.
                                    split(new RegExp('[\\'+scwArrDelimiters.
                                                        join('\\')+']+','g'));

                 // "Escape" all the user defined date delimiters above -
                 // several delimiters will need it and it does no harm for
                 // the others.

                 // Strip any empty array elements (caused by delimiters)
                 // from the beginning or end of the array. They will
                 // still appear in the output string if in the output
                 // format.

                 if (scwArrInput[0].length==0) scwArrInput.splice(0,1);

                 if (scwArrInput[scwArrInput.length-1].length==0)
                    scwArrInput.splice(scwArrInput.length-1,1);

                 scwBlnFullInputDate = false;

                 switch (scwArrInput.length)
                    {case 1:
                        {// Year only entry
                         scwArrSeed[0] = parseInt(scwArrInput[0],10);   // Year
                         scwArrSeed[1] = '1';                           // Month
                         scwArrSeed[2] = 1;                             // Day
                         break;
                        }
                     case 2:
                        {// Year and Month entry
                         scwArrSeed[0] =
                             parseInt(scwArrInput[scwDateInputSequence.
                                                    replace(/D/i,'').
                                                    search(/Y/i)],10);  // Year
                         scwArrSeed[1] = scwArrInput[scwDateInputSequence.
                                                    replace(/D/i,'').
                                                    search(/M/i)];      // Month
                         scwArrSeed[2] = 1;                             // Day
                         break;
                        }
                     case 3:
                     case 4:
                     case 5:
                        {// Day Month and Year entry + optional hour & minute

                         scwArrSeed[0] =
                             parseInt(scwArrInput[scwDateInputSequence.
                                                    search(/Y/i)],10);  // Year
                         scwArrSeed[1] = scwArrInput[scwDateInputSequence.
                                                    search(/M/i)];      // Month
                         scwArrSeed[2] =
                             parseInt(scwArrInput[scwDateInputSequence.
                                                    search(/D/i)],10);  // Day

                         scwBlnFullInputDate = true;

						 // Handle optional hour & minute
						 if(scwArrInput.length > 3)
						 {
							scwSelHour = scwArrInput[3].scwPadLeft(2);
							scwSelMin = scwArrInput.length == 4 ? '00' : scwArrInput[4].scwPadLeft(2);
						 }
						 else
						 {
							scwSelHour = '';
							scwSelMin = '';
						 }
                         break;
                        }
                     default:
                        {// A stuff-up has led to more than three elements in
                         // the date.
                         scwArrSeed[0] = 0;     // Year
                         scwArrSeed[1] = 0;     // Month
                         scwArrSeed[2] = 0;     // Day
                        }
                    }

                 // These regular expressions validate the input date format
                 // to the following rules;
                 //         Day   1-31 (optional zero on single digits)
                 //         Month 1-12 (optional zero on single digits)
                 //                     or case insensitive name
                 //         Year  One, Two or four digits

                 // Months names are as set in the language-dependent
                 // definitions and delimiters are set just below there

                 var scwExpValDay    = /^(0?[1-9]|[1-2]\d|3[0-1])$/,
                     scwExpValMonth  = new RegExp("^(0?[1-9]|1[0-2]|"        +
                                                  scwArrMonthNames.join("|") +
                                                  ")$","i"),
                     scwExpValYear   = /^(\d{1,2}|\d{4})$/;

                 // Apply validation and report failures

                 if (scwExpValYear.exec(scwArrSeed[0])  == null ||
                     scwExpValMonth.exec(scwArrSeed[1]) == null ||
                     scwExpValDay.exec(scwArrSeed[2])   == null)
                     {alert(scwInvalidDateMsg  +
                            scwInvalidAlert[0] + scwEleValue +
                            scwInvalidAlert[1]);
                      scwBlnFullInputDate = false;
                      scwArrSeed[0] = scwBaseYear +
                                      Math.floor(scwDropDownYears/2); // Year
                      scwArrSeed[1] = '6';                            // Month
                      scwArrSeed[2] = 1;                              // Day
                     }

                 // Return the  Year    in scwArrSeed[0]
                 //             Month   in scwArrSeed[1]
                 //             Day     in scwArrSeed[2]

                 return scwArrSeed;
                }

             // Parse the string into an array using the allowed delimiters

             var scwArrSeedDate = scwInputFormat(scwEle.value);

             // So now we have the Year, Month and Day in an array.

             //   If the year is one or two digits then the routine assumes a
             //   year belongs in the 21st Century unless it is less than 50
             //   in which case it assumes the 20th Century is intended.

             if (scwArrSeedDate[0]<100)
                scwArrSeedDate[0] += (scwArrSeedDate[0]>50)?1900:2000;

             // Check whether the month is in digits or an abbreviation

             if (scwArrSeedDate[1].search(/\d+/)!=0)
                {month = scwArrMonthNames.join('|').toUpperCase().
                            search(scwArrSeedDate[1].substr(0,3).
                                                    toUpperCase());
                 scwArrSeedDate[1] = Math.floor(month/4)+1;
                }

             scwSeedDate = new Date(scwArrSeedDate[0],
                                    scwArrSeedDate[1]-1,
                                    scwArrSeedDate[2]);
            }

         // Test that we have arrived at a valid date

         if (isNaN(scwSeedDate))
            {alert( scwInvalidDateMsg +
                    scwInvalidAlert[0] + scwEle.value +
                    scwInvalidAlert[1]);
             scwSeedDate = new Date(scwBaseYear +
                    Math.floor(scwDropDownYears/2),5,1);
             scwBlnFullInputDate=false;
            }
         else
            {// Test that the date is within range,
             // if not then set date to a sensible date in range.

             if ((new Date(scwBaseYear,0,1)) > scwSeedDate)
                {if (scwBlnStrict) alert(scwOutOfRangeMsg);
                 scwSeedDate = new Date(scwBaseYear,0,1);
                 scwBlnFullInputDate=false;
                }
             else
                {if ((new Date(scwBaseYear+scwDropDownYears-1,11,31))<
                      scwSeedDate)
                    {if (scwBlnStrict) alert(scwOutOfRangeMsg);
                     scwSeedDate = new Date(scwBaseYear +
                                            Math.floor(scwDropDownYears)-1,
                                                       11,1);
                     scwBlnFullInputDate=false;
                    }
                 else
                    {if (scwBlnStrict && scwBlnFullInputDate &&
                          (scwSeedDate.getDate()      != scwArrSeedDate[2] ||
                           (scwSeedDate.getMonth()+1) != scwArrSeedDate[1] ||
                           scwSeedDate.getFullYear()  != scwArrSeedDate[0]
                          )
                        )
                        {alert(scwDoesNotExistMsg);
                         scwSeedDate = new Date(scwSeedDate.getFullYear(),
                                                scwSeedDate.getMonth()-1,1);
                         scwBlnFullInputDate=false;
                        }
                    }
                }
            }

         // Test the disabled dates for validity
         // Give error message if not valid.

         for (var i=0;i<scwDisabledDates.length;i++)
            {if (!((typeof scwDisabledDates[i]      == 'object') &&
                   (scwDisabledDates[i].constructor == Date)))
                {if ((typeof scwDisabledDates[i]      == 'object') &&
                     (scwDisabledDates[i].constructor == Array))
                    {var scwPass = true;

                     if (scwDisabledDates[i].length !=2)
                        {//alert(scwRangeDisablingError[0] +
                         //      scwDisabledDates[i] +
                         //      scwRangeDisablingError[1]);
                         scwPass = false;
                        }
                     else
                        {for (var j=0;j<scwDisabledDates[i].length;j++)
                            {if (!((typeof scwDisabledDates[i][j]
                                    == 'object') &&
                                   (scwDisabledDates[i][j].constructor
                                    == Date)))
                                {//alert(scwDateDisablingError[0] +
                                 //      scwDisabledDates[i][j] +
                                 //      scwDateDisablingError[1]);
                                 scwPass = false;
                                }
                            }
                        }

                     if (scwPass &&
                         (scwDisabledDates[i][0] > scwDisabledDates[i][1])
                        )
                        {scwDisabledDates[i].reverse();}
                    }
//               else
//                  {alert(scwDateDisablingError[0] + scwDisabledDates[i] +
//                         scwDateDisablingError[1]);}
                }
            }

         // Calculate the number of months that the entered (or
         // defaulted) month is after the start of the allowed
         // date range.

         scwMonthSum =  12*(scwSeedDate.getFullYear()-scwBaseYear)+
                            scwSeedDate.getMonth();

         // Set the drop down boxes.

         document.getElementById('scwYears').options.selectedIndex =
            Math.floor(scwMonthSum/12);
         document.getElementById('scwMonths').options.selectedIndex=
            (scwMonthSum%12);

         scwTargetEle=scwEle;

		// Set option hour / min boxes
		if(scwSelHour != '') {
			if(scwSelMin == '') scwSelMin = "00";
			var el = document.getElementById('scwHr' + scwSelHour);
			if(el) scwChangeHourMin(0, el);
			el = document.getElementById('scwMn' + scwSelMin);
			if(el) scwChangeHourMin(1, el);
		}

         // Position the calendar box

         var offsetTop =parseInt(scwEle.offsetTop ,10) +
                        parseInt(scwEle.offsetHeight,10),
             offsetLeft=parseInt(scwEle.offsetLeft,10);

         do {scwEle=scwEle.offsetParent;
             offsetTop +=parseInt(scwEle.offsetTop,10);
             offsetLeft+=parseInt(scwEle.offsetLeft,10);
            }
         while (scwEle.tagName!='BODY' && scwEle.tagName!='HTML');

		if(document.body.offsetWidth - offsetLeft < 220)
			{offsetLeft -= 220  - document.body.scrollLeft - (document.body.offsetWidth - offsetLeft);}

         document.getElementById('scw').style.top =offsetTop +'px';
         document.getElementById('scw').style.left=offsetLeft+'px';

         if (document.getElementById('scwIframe'))
            {document.getElementById('scwIframe').style.top=offsetTop +'px';
             document.getElementById('scwIframe').style.left=offsetLeft+'px';
             document.getElementById('scwIframe').style.width=
                (document.getElementById('scw').offsetWidth-2)+'px';
             document.getElementById('scwIframe').style.height=
                (document.getElementById('scw').offsetHeight-2)+'px';
             document.getElementById('scwIframe').style.visibility='visible';
            }

         // Check whether or not dragging is allowed and display drag handle
         // if necessary

         document.getElementById('scwDrag').style.display=
             (scwAllowDrag)
                ?((document.getElementById('scwIFrame')||
                   document.getElementById('scwIEgte7'))?'block':'table-row')
                :'none';

         // Display the month

         scwShowMonth(0);

         // Show it on the page

         document.getElementById('scw').style.visibility='visible';

         if (typeof event=='undefined')
                {scwSourceEle.parentNode.
                        addEventListener("click",scwStopPropagation,false);
                }
         else   {event.cancelBubble = true;}
        }

    function scwHide()
        {document.getElementById('scw').style.visibility='hidden';
         if (document.getElementById('scwIframe'))
            {document.getElementById('scwIframe').style.visibility='hidden';}

         if (typeof scwNextAction!='undefined' && scwNextAction!=null)
             {scwNextActionReturn = scwNextAction();
              // Explicit null set to prevent closure causing memory leak
              scwNextAction = null;
             }
        }

    function scwCancel(scwEvt)
        {if (scwClickToHide) scwHide();
         scwStopPropagation(scwEvt);
        }

    function scwStopPropagation(scwEvt)
        {if (scwEvt.stopPropagation)
                scwEvt.stopPropagation();    // Capture phase
         else   scwEvt.cancelBubble = true;  // Bubbling phase
        }

    function scwBeginDrag(event)
        {var elementToDrag = document.getElementById('scw');

         var deltaX    = event.clientX,
             deltaY    = event.clientY,
             offsetEle = elementToDrag;

         do {deltaX   -= parseInt(offsetEle.offsetLeft,10);
             deltaY   -= parseInt(offsetEle.offsetTop ,10);
             offsetEle = offsetEle.offsetParent;
            }
         while (offsetEle.tagName!='BODY' &&
                offsetEle.tagName!='HTML');

         if (document.addEventListener)
                {document.addEventListener('mousemove',
                                           moveHandler,
                                           true);        // Capture phase
                 document.addEventListener('mouseup',
                                           upHandler,
                                           true);        // Capture phase
                }
         else   {elementToDrag.attachEvent('onmousemove',
                                           moveHandler); // Bubbling phase
                 elementToDrag.attachEvent('onmouseup',
                                             upHandler); // Bubbling phase
                 elementToDrag.setCapture();
                }

         scwStopPropagation(event);

         function moveHandler(e)
            {if (!e) e = window.event;

             elementToDrag.style.left = (e.clientX - deltaX) + 'px';
             elementToDrag.style.top  = (e.clientY - deltaY) + 'px';

             if (document.getElementById('scwIframe'))
                {document.getElementById('scwIframe').style.left =
                    (e.clientX - deltaX) + 'px';
                 document.getElementById('scwIframe').style.top  =
                    (e.clientY - deltaY) + 'px';
                }

             scwStopPropagation(e);
            }

         function upHandler(e)
            {if (!e) e = window.event;

             if (document.removeEventListener)
                    {document.removeEventListener('mousemove',
                                                  moveHandler,
                                                  true);     // Capture phase
                     document.removeEventListener('mouseup',
                                                  upHandler,
                                                  true);     // Capture phase
                    }
             else   {elementToDrag.detachEvent('onmouseup',
                                                 upHandler); // Bubbling phase
                     elementToDrag.detachEvent('onmousemove',
                                               moveHandler); // Bubbling phase
                     elementToDrag.releaseCapture();
                    }

             scwStopPropagation(e);
            }
        }
		
    function scwShowMonth(scwBias)
        {// Set the selectable Month and Year
         // May be called: from the left and right arrows
         //                  (shift month -1 and +1 respectively)
         //                from the month selection list
         //                from the year selection list
         //                from the showCal routine
         //                  (which initiates the display).

         var scwShowDate  = new Date(Date.parse(new Date().toDateString())),
             scwStartDate = new Date(),
             scwSaveBackground,
             scwSaveText;

         var scwSelYears  = document.getElementById('scwYears');
         var scwSelMonths = document.getElementById('scwMonths');

         if (scwSelYears.options.selectedIndex>-1)
            {scwMonthSum=12*(scwSelYears.options.selectedIndex)+scwBias;
             if (scwSelMonths.options.selectedIndex>-1)
                {scwMonthSum+=scwSelMonths.options.selectedIndex;}
            }
         else
            {if (scwSelMonths.options.selectedIndex>-1)
                {scwMonthSum+=scwSelMonths.options.selectedIndex;}
            }

         scwShowDate.setFullYear(scwBaseYear + Math.floor(scwMonthSum/12),
                                 (scwMonthSum%12),
                                 1);

         // If the Week numbers are displayed, shift the week day names
         // to the right.
         document.getElementById("scwWeek_").style.display=
             (scwWeekNumberDisplay)?'block':'none';

         if ((12*parseInt((scwShowDate.getFullYear()-scwBaseYear),10)) +
             parseInt(scwShowDate.getMonth(),10) < (12*scwDropDownYears)  &&
             (12*parseInt((scwShowDate.getFullYear()-scwBaseYear),10)) +
             parseInt(scwShowDate.getMonth(),10) > -1)
            {scwSelYears.options.selectedIndex=Math.floor(scwMonthSum/12);
             scwSelMonths.options.selectedIndex=(scwMonthSum%12);

             var scwCurMonth = scwShowDate.getMonth();

             scwShowDate.setDate((((scwShowDate.
                                    getDay()-scwWeekStart)<0)?-6:1)+
                                 scwWeekStart-scwShowDate.getDay());

             scwStartDate = new Date(scwShowDate);

             var scwFoot = document.getElementById('scwFoot');

             function scwFootOutput()   {scwSetOutput(scwDateNow);}

             function scwFootOver()
                {document.getElementById('scwFoot').style.color=
                    scwTodayHighlight;
                 document.getElementById('scwFoot').style.fontWeight='bold';
                }

             function scwFootOut()
                {document.getElementById('scwFoot').style.color=scwTodayText;
                 document.getElementById('scwFoot').style.fontWeight='normal';
                }

             if (scwDisabledDates.length==0)
                {if (scwActiveToday && scwParmActiveToday)
                    {scwFoot.onclick     =scwFootOutput;
                     scwFoot.onmouseover =scwFootOver;
                     scwFoot.onmouseout  =scwFootOut;
                     scwFoot.style.cursor=
                         (document.getElementById('scwIFrame')||
                          document.getElementById('scwIEgte7'))
                             ?'hand':'pointer';
                    }
                 else
                    {scwFoot.onclick     =null;
                     if (document.addEventListener)
                            {scwFoot.addEventListener('click',
                                                      scwStopPropagation,
                                                      false);}
                     else   {scwFoot.attachEvent('onclick',
                                                 scwStopPropagation);}
                     scwFoot.onmouseover =null;
                     scwFoot.onmouseout  =null;
                     scwFoot.style.cursor='default';
                    }
                }
             else
                {for (var k=0;k<scwDisabledDates.length;k++)
                    {if (!scwActiveToday || !scwParmActiveToday ||
                         ((typeof scwDisabledDates[k] == 'object')            &&
                             (((scwDisabledDates[k].constructor == Date)      &&
                               scwDateNow.valueOf() == scwDisabledDates[k].
                                                            valueOf()
                              ) ||
                              ((scwDisabledDates[k].constructor == Array)     &&
                               scwDateNow.valueOf() >= scwDisabledDates[k][0].
                                                        valueOf()             &&
                               scwDateNow.valueOf() <= scwDisabledDates[k][1].
                                                        valueOf()
                              )
                             )
                         )
                        )
                        {scwFoot.onclick     =null;
                         if (document.addEventListener)
                                {scwFoot.addEventListener('click',
                                                          scwStopPropagation,
                                                          false);
                                }
                         else   {scwFoot.attachEvent('onclick',
                                                     scwStopPropagation);
                                }
                         scwFoot.onmouseover =null;
                         scwFoot.onmouseout  =null;
                         scwFoot.style.cursor='default';
                         break;
                        }
                     else
                        {scwFoot.onclick     =scwFootOutput;
                         scwFoot.onmouseover =scwFootOver;
                         scwFoot.onmouseout  =scwFootOut;
                         scwFoot.style.cursor=
                             (document.getElementById('scwIFrame')||
                              document.getElementById('scwIEgte7'))
                                  ?'hand':'pointer';
                        }
                    }
                }

            function scwSetOutput(scwOutputDate)
            {
				var h = scwSelHour;
				var m = scwSelMin;
				scwTargetEle.value = scwOutputDate.scwFormat(scwDateOutputFormat) + (h == '' ? '' : ' ' + h + ':' + (m == '' ? '00' : m));
			// Modif du 09/03/2009 pour changement de style de fonctionnement
			//	scwHide();
            }

             function scwCellOutput(scwEvt)
                {var scwEle = scwEventTrigger(scwEvt),
                     scwOutputDate = new Date(scwStartDate);

                 if (scwEle.nodeType==3) scwEle=scwEle.parentNode;

                 scwOutputDate.setDate(scwStartDate.getDate() +
                                         parseInt(scwEle.id.substr(8),10));

                 scwSetOutput(scwOutputDate);
//				 alert(scwTargetEle.value);
				 showCal(scwTargetEle,scwTargetEle);
                }

             function scwHighlight(e)
                {var scwEle = scwEventTrigger(e);

                 if (scwEle.nodeType==3) scwEle=scwEle.parentNode;

                 scwSaveText        =scwEle.style.color;
                 scwSaveBackground  =scwEle.style.backgroundColor;

                 scwEle.style.color             =scwHighlightText;
                 scwEle.style.backgroundColor   =scwHighlightBackground;

                 return true;
                }

             function scwUnhighlight(e)
                {var scwEle = scwEventTrigger(e);

                 if (scwEle.nodeType==3) scwEle =scwEle.parentNode;

                 scwEle.style.backgroundColor   =scwSaveBackground;
                 scwEle.style.color             =scwSaveText;

                 return true;
                }

             function scwEventTrigger(e)
                {if (!e) e = event;
                 return e.target||e.srcElement;
                }

            function scwWeekNumber(scwInDate)
                {// The base day in the week of the input date
                 var scwInDateWeekBase = new Date(scwInDate);

                 scwInDateWeekBase.setDate(scwInDateWeekBase.getDate()
                                            - scwInDateWeekBase.getDay()
                                            + scwWeekNumberBaseDay
                                            + ((scwInDate.getDay()>
                                                scwWeekNumberBaseDay)?7:0));

                 // The first Base Day in the year
                 var scwFirstBaseDay = new Date(scwInDateWeekBase.getFullYear(),0,1)

                 scwFirstBaseDay.setDate(scwFirstBaseDay.getDate()
                                            - scwFirstBaseDay.getDay()
                                            + scwWeekNumberBaseDay
                                        );

                 if (scwFirstBaseDay < new Date(scwInDateWeekBase.getFullYear(),0,1))
                    {scwFirstBaseDay.setDate(scwFirstBaseDay.getDate()+7);}

                 // Start of Week 01
                 var scwStartWeekOne = new Date(scwFirstBaseDay
                                                - scwWeekNumberBaseDay
                                                + scwInDate.getDay());

                 if (scwStartWeekOne > scwFirstBaseDay)
                    {scwStartWeekOne.setDate(scwStartWeekOne.getDate()-7);}

                 // Subtract the date of the current week from the date of the
                 // first week of the year to get the number of weeks in
                 // milliseconds.  Divide by the number of milliseconds
                 // in a week then round to no decimals in order to remove
                 // the effect of daylight saving.  Add one to make the first
                 // week, week 1.  Place a string zero on the front so that
                 // week numbers are zero filled.

                 var scwWeekNo = "0" + (Math.round((scwInDateWeekBase -
                                                    scwFirstBaseDay)/604800000,0) + 1);

                 // Return the last two characters in the week number string

                 return scwWeekNo.substring(scwWeekNo.length-2,scwWeekNo.length);
                }

             // Treewalk to display the dates.
             // I tried to use getElementsByName but IE refused to cooperate
             // so I resorted to this method which works for all tested
             // browsers.

             var scwCells = document.getElementById('scwCells');

             for (i=0;i<scwCells.childNodes.length;i++)
                {var scwRows = scwCells.childNodes[i];
                 if (scwRows.nodeType==1 && scwRows.tagName=='TR')
                    {if (scwWeekNumberDisplay)
                        {//Calculate the week number using scwShowDate
                         scwRows.childNodes[0].innerHTML=scwWeekNumber(scwShowDate);
                         scwRows.childNodes[0].style.display='block';
                        }
                     else
                        {scwRows.childNodes[0].style.display='none';}

                     for (j=1;j<scwRows.childNodes.length;j++)
                        {var scwCols = scwRows.childNodes[j];
                         if (scwCols.nodeType==1 && scwCols.tagName=='TD')
                            {scwRows.childNodes[j].innerHTML=
                                scwShowDate.getDate();
                             var scwCellStyle=scwRows.childNodes[j].style,
                                 scwDisabled =
                                    (scwOutOfRangeDisable &&
                                     (scwShowDate < (new Date(scwBaseYear,0,1))
                                      ||
                                      scwShowDate > (new Date(scwBaseYear+
                                                              scwDropDownYears-
                                                              1,11,31))
                                     )
                                    )?true:false;

                             for (var k=0;k<scwDisabledDates.length;k++)
                                {if ((typeof scwDisabledDates[k]=='object')
                                     &&
                                     (scwDisabledDates[k].constructor ==
                                      Date
                                     )
                                     &&
                                     scwShowDate.valueOf() ==
                                        scwDisabledDates[k].valueOf()
                                    )
                                    {scwDisabled = true;}
                                 else
                                    {if ((typeof scwDisabledDates[k]=='object')
                                         &&
                                         (scwDisabledDates[k].constructor ==
                                          Array
                                         )
                                         &&
                                         scwShowDate.valueOf() >=
                                             scwDisabledDates[k][0].valueOf()
                                         &&
                                         scwShowDate.valueOf() <=
                                             scwDisabledDates[k][1].valueOf()
                                        )
                                        {scwDisabled = true;}
                                    }
                                }

                             if (scwDisabled ||
                                 !scwEnabledDay[j-1+(7*((i*scwCells.
                                                          childNodes.
                                                          length)/6))] ||
                                 !scwPassEnabledDay[(j-1+(7*(i*scwCells.
                                                               childNodes.
                                                               length/6)))%7]
                                )
                                {scwRows.childNodes[j].onclick     =null;
                                 scwRows.childNodes[j].onmouseover =null;
                                 scwRows.childNodes[j].onmouseout  =null;
                                 scwRows.childNodes[j].style.cursor='default';

                                 if (scwShowDate.getMonth()!=scwCurMonth)
                                    {scwCellStyle.color=scwDisabledExMonthText;
                                     scwCellStyle.backgroundColor=
                                         scwDisabledExMonthBackground;
                                    }
                                 else if (scwBlnFullInputDate &&
                                          scwShowDate.toDateString()==
                                          scwSeedDate.toDateString())
                                    {scwCellStyle.color=scwDisabledInDateText;
                                     scwCellStyle.backgroundColor=
                                         scwDisabledInDateBackground;
                                    }
                                 else if (scwShowDate.getDay()%6==0)
                                    {scwCellStyle.color=scwDisabledWeekendText;
                                     scwCellStyle.backgroundColor=
                                         scwDisabledWeekendBackground;
                                    }
                                 else
                                    {scwCellStyle.color=scwDisabledCellText;
                                     scwCellStyle.backgroundColor=
                                         scwDisabledCellBackground;
                                    }
                                }
                             else
                                {scwRows.childNodes[j].onclick      =
                                    scwCellOutput;
                                 scwRows.childNodes[j].onmouseover  =
                                    scwHighlight;
                                 scwRows.childNodes[j].onmouseout   =
                                    scwUnhighlight;
                                 scwRows.childNodes[j].style.cursor =
                                    (document.getElementById('scwIFrame')||
                                     document.getElementById('scwIEgte7'))
                                        ?'hand':'pointer';

                                 if (scwShowDate.getMonth()!=scwCurMonth)
                                    {scwCellStyle.color=scwExMonthText;
                                     scwCellStyle.backgroundColor=
                                         scwExMonthBackground;
                                    }
                                 else if (scwBlnFullInputDate &&
                                          scwShowDate.toDateString()==
                                          scwSeedDate.toDateString())
                                    {scwCellStyle.color=scwInDateText;
                                     scwCellStyle.backgroundColor=
                                         scwInDateBackground;
                                    }
                                 else if (scwShowDate.getDay()%6==0)
                                    {scwCellStyle.color=scwWeekendText;
                                     scwCellStyle.backgroundColor=
                                         scwWeekendBackground;
                                    }
                                 else
                                    {scwCellStyle.color=scwCellText;
                                     scwCellStyle.backgroundColor=
                                         scwCellBackground;
                                    }
                                }

                             scwShowDate.setDate(scwShowDate.getDate()+1);
                            }
                        }
                    }
                }
            }
        }


// Time input handling functions
var scwHrElPrev = 0, scwMnElPrev = 0;
function scwHLHourMin(b_in,el) 
{
	el.style.backgroundColor = b_in ? scwHighlightBackground :
						el == scwHrElPrev || el == scwMnElPrev ? scwInDateBackground : scwCellBackground;
}
function scwChangeHourMin(b_mn,el) 
{
	var prev_el = b_mn ? scwMnElPrev : scwHrElPrev;
	if(prev_el) {
		prev_el.style.backgroundColor = scwCellBackground;
		prev_el.style.color = scwCellText;
	}
	el.style.backgroundColor = scwInDateBackground;
	el.style.color=scwInDateText;
	if(b_mn) {
		scwSelMin = el.innerHTML;
		scwMnElPrev = el;
	} else {
		scwSelHour = el.innerHTML;
		scwHrElPrev = el;
	}
	var h = scwSelHour;
	var m = scwSelMin;
	if(scwTargetEle.value.length < 8) scwTargetEle.value = scwDateNow.scwFormat(scwDateDisplayFormat);
	scwTargetEle.value = scwTargetEle.value.substring(0,10) + (h == '' ? '' : ' ' + h + ':' + (m == '' ? '00' : m));
}

// *************************
//  End of Function Library
// *************************
// ***************************
// Start of Calendar structure
// ***************************

    document.write(
     "<!--[if gte IE 7]>" +
        "<div id='scwIEgte7'></div>" +
     "<![endif]-->" +
     "<!--[if lt  IE 7]>" +
        "<iframe class='scw' src='scwblank.html' " +
                "id='scwIframe' name='scwIframe' " +
                "frameborder='0'>" +
        "</iframe>" +
     "<![endif]-->" +
     "<table id='scw' class='scw' onclick='scwCancel(event);'>" +
       "<tr class='scw'>" +
         "<td class='scw'>" +
           "<table class='scwHead' id='scwHead' width='100%' " +
                    "onClick='scwStopPropagation(event);' " +
                    "cellspacing='0' cellpadding='0'>" +
            "<tr id='scwDrag' style='display:none;'>" +
                "<td colspan='4' class='scwDrag' " +
                    "onmousedown='scwBeginDrag(event);'>" +
                    "<div id='scwDragText'></div>" +
                "</td>" +
            "</tr>" +
            "<tr class='scwHead'>" +
                 "<td class='scwHead'>" +
                    "<input class='scwHead' type='button' value='<' " +
                            "onclick='scwShowMonth(-1);'  /></td>" +
                 "<td class='scwHead'>" +
                    "<select id='scwMonths' class='scwHead' " +
                            "onChange='scwShowMonth(0);'>" +
                    "</select>" +
                 "</td>" +
                 "<td class='scwHead'>" +
                    "<select id='scwYears' class='scwHead' " +
                            "onChange='scwShowMonth(0);'>" +
                    "</select>" +
                 "</td>" +
                 "<td class='scwHead'>" +
                    "<input class='scwHead' type='button' value='>' " +
                            "onclick='scwShowMonth(1);' /></td>" +
                "</tr>" +
              "</table>" +
            "</td>" +
          "</tr>" +
          "<tr class='scw'>" +
            "<td class='scw'>" +
              "<table class='scwCells' align='center'>" +
                "<thead>" +
                  "<tr><td class='scwWeekNumberHead' id='scwWeek_' ></td>");

    for (var i=0;i<7;i++)
        document.write( "<td class='scwWeek' id='scwWeekInit" + i + "'></td>");

    document.write("</tr>" +
                "</thead>" +
                "<tbody id='scwCells' " +
                        "onClick='scwStopPropagation(event);'>");

    for (var i=0;i<6;i++)
        {document.write(
                    "<tr>" +
                      "<td class='scwWeekNo' id='scwWeek_" + i + "'></td>");
         for (var j=0;j<7;j++)
            {document.write(
                        "<td class='scwCells' id='scwCell_" + (j+(i*7)) +
                        "'></td>");
            }

         document.write(
                    "</tr>");
        }

    document.write(
                "</tbody>");

    if ((new Date(scwBaseYear + scwDropDownYears, 11, 32)) > scwDateNow &&
        (new Date(scwBaseYear, 0, 0))                      < scwDateNow)
        {document.write(
                  "<tfoot class='scwFoot'>" +
                    "<tr class='scwFoot'>" +
                      "<td class='scwFoot' id='scwFoot' colspan='8'>" +
                      "</td>" +
                    "</tr>" +
                  "</tfoot>");
        }

    document.write(
              "</table>" +
            "</td>" +
          "</tr>");

	// Hour input
    document.write(
		"<tr><td><table class='scwCells' align='center'>" +
		"<tr><td class='scwTime' rowspan=4><b>H<br>e<br>u<br>r<br>e</b></td>");
	for(var i = 0; i < 4; i++)
	{
		for(var j = 0; j < 6; j++) {
			var k = i*6+j;
			k = (k < 10 ? "0" : "") + k;
			document.write(
				"<td id=scwHr" + k + " class='scwTimeCell' style='cursor:pointer' " +
					"onClick='scwChangeHourMin(0,this);' " +
					"onmouseover='scwHLHourMin(1,this);' " +
					"onmouseout='scwHLHourMin(0,this);' >" + k + "</td>");
		}
		document.write(
			"</tr><tr>");
	}

	// Time input
    document.write(
                "</tr><tr><td class='scwTime' rowspan=2><b>M<br>n</b></td>");
	for(var i = 0; i < 2; i++)
	{
		for(var j = 0; j < 6; j++) {
			var k = i*30+j*5;
			k = (k < 10 ? "0" : "") + k;
			document.write(
				"<td id=scwMn" + k + " class='scwTimeCell' style='cursor:pointer' " +
					"onClick='scwChangeHourMin(1,this);' " +
					"onmouseover='scwHLHourMin(1,this);' " +
					"onmouseout='scwHLHourMin(0,this);' >" + k + "</td>");
		}
		document.write(
			"</tr><tr>");
	}
    document.write(
			"</tr></table></td></tr>"+
		"</table>");

// ***************************
//  End of Calendar structure
// ***************************
// ****************************************
// Start of document level event definition
// ****************************************

    if (document.addEventListener)
            {document.addEventListener('click',scwHide, false);}
    else    {document.attachEvent('onclick',scwHide);}

// ****************************************
//  End of document level event definition
// ****************************************
// ************************************
//  End of Simple Calendar Widget Code
// ************************************