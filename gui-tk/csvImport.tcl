# assume canvas.minsky.item is the variable
package require http
CSVDialog csvDialog
proc CSVImportDialog {} {
    global workDir csvParms
    if {![winfo exists .wiring.csvImport]} {
        toplevel .wiring.csvImport
        frame .wiring.csvImport.delimiters
        label .wiring.csvImport.delimiters.columnarLabel -text "Columnar"
        ttk::checkbutton .wiring.csvImport.delimiters.columnar -variable csvParms(columnar) -command {
            csvDialog.spec.columnar $csvParms(columnar)
            csvDialog.requestRedraw
        }
        label .wiring.csvImport.delimiters.separatorLabel -text Separator
        ttk::combobox .wiring.csvImport.delimiters.separatorValue -values {
            "," ";" "<tab>" "<space>"} -textvariable csvParms(separator) -width 5
        bind .wiring.csvImport.delimiters.separatorValue <<ComboboxSelected>> {
            csvDialog.spec.separator $csvParms(separator)}
        label .wiring.csvImport.delimiters.decSeparatorLabel -text "Decimal Separator"
        ttk::combobox .wiring.csvImport.delimiters.decSeparatorValue -values {
            "." ","} -textvariable csvParms(decSeparator) -width 5
        bind .wiring.csvImport.delimiters.decSeparatorValue <<ComboboxSelected>> {
            csvDialog.spec.decSeparator $csvParms(decSeparator)}
        label .wiring.csvImport.delimiters.escapeLabel -text Escape
        ttk::combobox .wiring.csvImport.delimiters.escapeValue -values {
            "\\"} -textvariable csvParms(escape) -width 5
        bind .wiring.csvImport.delimiters.escapeValue <<ComboboxSelected>> {
            csvDialog.spec.escape $csvParms(escape)}
        label .wiring.csvImport.delimiters.quoteLabel -text Quote
        ttk::combobox .wiring.csvImport.delimiters.quoteValue -values {
            "'" "\"" } -textvariable csvParms(quote) -width 5
        bind .wiring.csvImport.delimiters.quoteValue <<ComboboxSelected>> {
            csvDialog.spec.quote $csvParms(quote)}
        label .wiring.csvImport.delimiters.mergeLabel -text "Merge Delimiters"
        ttk::checkbutton .wiring.csvImport.delimiters.mergeValue -variable csvParms(mergeDelimiters) -command {
            csvDialog.spec.mergeDelimiters $csvParms(mergeDelimiters)
        }
        label .wiring.csvImport.delimiters.missingLabel -text "Missing Value"
        ttk::combobox .wiring.csvImport.delimiters.missingValue \
            -textvariable csvParms(missingValue) -values {nan 0} -width 5
        bind .wiring.csvImport.delimiters.missingValue <<ComboboxSelected>> {
            csvDialog.spec.missingValue $csvParms(missingValue)}

        label .wiring.csvImport.delimiters.colWidthLabel -text "Col Width"
        spinbox .wiring.csvImport.delimiters.colWidth -from 10 -to 500 -increment 10 \
            -width 5 -command {adjustColWidth %s} -validate key -validatecommand {adjustColWidth %P}
        .wiring.csvImport.delimiters.colWidth set 50
        
        pack .wiring.csvImport.delimiters.columnarLabel .wiring.csvImport.delimiters.columnar \
            .wiring.csvImport.delimiters.separatorLabel .wiring.csvImport.delimiters.separatorValue \
            .wiring.csvImport.delimiters.decSeparatorLabel .wiring.csvImport.delimiters.decSeparatorValue \
            .wiring.csvImport.delimiters.escapeLabel .wiring.csvImport.delimiters.escapeValue \
            .wiring.csvImport.delimiters.quoteLabel .wiring.csvImport.delimiters.quoteValue \
            .wiring.csvImport.delimiters.mergeLabel .wiring.csvImport.delimiters.mergeValue \
            .wiring.csvImport.delimiters.missingLabel .wiring.csvImport.delimiters.missingValue \
            .wiring.csvImport.delimiters.colWidthLabel .wiring.csvImport.delimiters.colWidth -side left

        pack .wiring.csvImport.delimiters
        
        frame .wiring.csvImport.horizontalName
        label .wiring.csvImport.horizontalName.text -text "Horizontal dimension"
        entry .wiring.csvImport.horizontalName.value -textvariable csvParms(horizontalDimension)

        label .wiring.csvImport.horizontalName.duplicateKeyLabel -text "Duplicate Key Action"
        ttk::combobox .wiring.csvImport.horizontalName.duplicateKeyValue \
            -textvariable csvParms(duplicateKeyValue) \
            -values {throwException sum product min max av} -width 15
        bind .wiring.csvImport.horizontalName.duplicateKeyValue <<ComboboxSelected>> {
            csvDialog.spec.duplicateKeyAction $csvParms(duplicateKeyValue)}

        pack .wiring.csvImport.horizontalName.duplicateKeyLabel .wiring.csvImport.horizontalName.duplicateKeyValue -side left
        pack .wiring.csvImport.horizontalName.text .wiring.csvImport.horizontalName.value -side left
        pack .wiring.csvImport.horizontalName

        image create cairoSurface csvDialogTable -surface csvDialog
        label .wiring.csvImport.table -image csvDialogTable -width 800 -height 300
        pack .wiring.csvImport.table -fill both -expand 1 -side top

        scale .wiring.csvImport.hscroll -orient horiz -from -100 -to 1000 -showvalue 0 -command scrollTable
        pack .wiring.csvImport.hscroll -fill x -expand 1 -side top
        
        buttonBar .wiring.csvImport {}
        # redefine OK command to not delete the the import window on error
        global csvImportFailed
        set csvImportFailed 0
        .wiring.csvImport.buttonBar.ok configure -command csvImportDialogOK
        bind .wiring.csvImport.table <Configure> "csvDialog.requestRedraw"
        bind .wiring.csvImport.table <Button-1> {csvImportButton1 %x %y};
        bind .wiring.csvImport.table <ButtonRelease-1> {csvImportButton1Up %x %y %X %Y};
        bind .wiring.csvImport.table <B1-Motion> {
            csvDialog.xoffs [expr $csvImportPanX+%x];
            if $movingHeader {
                set row [csvDialog.rowOver %y]
                if {$row>=4} {csvDialog.spec.headerRow [expr $row-4]}
                csvDialog.flashNameRow [expr $row==3]
            }
            csvDialog.requestRedraw
        }
    }
    set filename [tk_getOpenFile -filetypes {{CSV {.csv}} {All {.*}}} -initialdir $workDir]
    if [string length $filename] {
        set workDir [file dirname $filename]
        csvDialog.loadFile $filename
        set csvParms(filename) $filename
        set csvParms(separator) [csvDialog.spec.separator]
        set csvParms(decSeparator) [csvDialog.spec.decSeparator]
        set csvParms(escape) [csvDialog.spec.escape]
        set csvParms(quote) [csvDialog.spec.quote]
        set csvParms(mergeDelimiters) [csvDialog.spec.mergeDelimiters]
        set csvParms(missingValue) [csvDialog.spec.missingValue]
        set csvParms(duplicateKeyValue) [csvDialog.spec.duplicateKeyAction]
        set csvParms(horizontalDimension) [csvDialog.spec.horizontalDimName]
        .wiring.csvImport.delimiters.colWidth set [csvDialog.colWidth]
        wm deiconify .wiring.csvImport
        raise .wiring.csvImport
        csvDialog.requestRedraw
    }

}

proc CSVWebImportDialog {} {
    global workDir csvParms
    if {![winfo exists .wiring.csvImport]} {
        toplevel .wiring.csvImport
        frame .wiring.csvImport.delimiters
        label .wiring.csvImport.delimiters.columnarLabel -text "Columnar"
        ttk::checkbutton .wiring.csvImport.delimiters.columnar -variable csvParms(columnar) -command {
            csvDialog.spec.columnar $csvParms(columnar)
            csvDialog.requestRedraw
        }
        label .wiring.csvImport.delimiters.separatorLabel -text Separator
        ttk::combobox .wiring.csvImport.delimiters.separatorValue -values {
            "," ";" "<tab>" "<space>"} -textvariable csvParms(separator) -width 5
        bind .wiring.csvImport.delimiters.separatorValue <<ComboboxSelected>> {
            csvDialog.spec.separator $csvParms(separator)}
        label .wiring.csvImport.delimiters.decSeparatorLabel -text "Decimal Separator"
        ttk::combobox .wiring.csvImport.delimiters.decSeparatorValue -values {
            "." ","} -textvariable csvParms(decSeparator) -width 5
        bind .wiring.csvImport.delimiters.decSeparatorValue <<ComboboxSelected>> {
            csvDialog.spec.decSeparator $csvParms(decSeparator)}
        label .wiring.csvImport.delimiters.escapeLabel -text Escape
        ttk::combobox .wiring.csvImport.delimiters.escapeValue -values {
            "\\"} -textvariable csvParms(escape) -width 5
        bind .wiring.csvImport.delimiters.escapeValue <<ComboboxSelected>> {
            csvDialog.spec.escape $csvParms(escape)}
        label .wiring.csvImport.delimiters.quoteLabel -text Quote
        ttk::combobox .wiring.csvImport.delimiters.quoteValue -values {
            "'" "\"" } -textvariable csvParms(quote) -width 5
        bind .wiring.csvImport.delimiters.quoteValue <<ComboboxSelected>> {
            csvDialog.spec.quote $csvParms(quote)}
        label .wiring.csvImport.delimiters.mergeLabel -text "Merge Delimiters"
        ttk::checkbutton .wiring.csvImport.delimiters.mergeValue -variable csvParms(mergeDelimiters) -command {
            csvDialog.spec.mergeDelimiters $csvParms(mergeDelimiters)
        }
        label .wiring.csvImport.delimiters.missingLabel -text "Missing Value"
        ttk::combobox .wiring.csvImport.delimiters.missingValue \
            -textvariable csvParms(missingValue) -values {nan 0} -width 5
        bind .wiring.csvImport.delimiters.missingValue <<ComboboxSelected>> {
            csvDialog.spec.missingValue $csvParms(missingValue)}

        label .wiring.csvImport.delimiters.colWidthLabel -text "Col Width"
        spinbox .wiring.csvImport.delimiters.colWidth -from 10 -to 500 -increment 10 \
            -width 5 -command {adjustColWidth %s} -validate key -validatecommand {adjustColWidth %P}
        .wiring.csvImport.delimiters.colWidth set 50
        
        pack .wiring.csvImport.delimiters.columnarLabel .wiring.csvImport.delimiters.columnar \
            .wiring.csvImport.delimiters.separatorLabel .wiring.csvImport.delimiters.separatorValue \
            .wiring.csvImport.delimiters.decSeparatorLabel .wiring.csvImport.delimiters.decSeparatorValue \
            .wiring.csvImport.delimiters.escapeLabel .wiring.csvImport.delimiters.escapeValue \
            .wiring.csvImport.delimiters.quoteLabel .wiring.csvImport.delimiters.quoteValue \
            .wiring.csvImport.delimiters.mergeLabel .wiring.csvImport.delimiters.mergeValue \
            .wiring.csvImport.delimiters.missingLabel .wiring.csvImport.delimiters.missingValue \
            .wiring.csvImport.delimiters.colWidthLabel .wiring.csvImport.delimiters.colWidth -side left

        pack .wiring.csvImport.delimiters
        
        frame .wiring.csvImport.horizontalName
        label .wiring.csvImport.horizontalName.text -text "Horizontal dimension"
        entry .wiring.csvImport.horizontalName.value -textvariable csvParms(horizontalDimension)

        label .wiring.csvImport.horizontalName.duplicateKeyLabel -text "Duplicate Key Action"
        ttk::combobox .wiring.csvImport.horizontalName.duplicateKeyValue \
            -textvariable csvParms(duplicateKeyValue) \
            -values {throwException sum product min max av} -width 15
        bind .wiring.csvImport.horizontalName.duplicateKeyValue <<ComboboxSelected>> {
            csvDialog.spec.duplicateKeyAction $csvParms(duplicateKeyValue)}

        pack .wiring.csvImport.horizontalName.duplicateKeyLabel .wiring.csvImport.horizontalName.duplicateKeyValue -side left
        pack .wiring.csvImport.horizontalName.text .wiring.csvImport.horizontalName.value -side left
        pack .wiring.csvImport.horizontalName

        image create cairoSurface csvDialogTable -surface csvDialog
        label .wiring.csvImport.table -image csvDialogTable -width 800 -height 300
        pack .wiring.csvImport.table -fill both -expand 1 -side top

        scale .wiring.csvImport.hscroll -orient horiz -from -100 -to 1000 -showvalue 0 -command scrollTable
        pack .wiring.csvImport.hscroll -fill x -expand 1 -side top
        
        buttonBar .wiring.csvImport {}
        # redefine OK command to not delete the the import window on error
        global csvImportFailed
        set csvImportFailed 0
        .wiring.csvImport.buttonBar.ok configure -command csvWebImportDialogOK
        bind .wiring.csvImport.table <Configure> "csvDialog.requestRedraw"
        bind .wiring.csvImport.table <Button-1> {csvImportButton1 %x %y};
        bind .wiring.csvImport.table <ButtonRelease-1> {csvImportButton1Up %x %y %X %Y};
        bind .wiring.csvImport.table <B1-Motion> {
            csvDialog.xoffs [expr $csvImportPanX+%x];
            if $movingHeader {
                set row [csvDialog.rowOver %y]
                if {$row>=4} {csvDialog.spec.headerRow [expr $row-4]}
                csvDialog.flashNameRow [expr $row==3]
            }
            csvDialog.requestRedraw
        }
    }
       
    set urlfile [inputUrl]
    
    #set filename [tk_getOpenFile -filetypes {{CSV {.csv}} {All {.*}}} -initialfile [getFile $urlfile] ]  
    
    set filename [getFile $urlfile]
    
    if [string length $filename] {
        set workDir [file dirname $filename ]
        csvDialog.loadFile $filename
        set csvParms(filename) $filename
        set csvParms(separator) [csvDialog.spec.separator]
        set csvParms(decSeparator) [csvDialog.spec.decSeparator]
        set csvParms(escape) [csvDialog.spec.escape]
        set csvParms(quote) [csvDialog.spec.quote]
        set csvParms(mergeDelimiters) [csvDialog.spec.mergeDelimiters]
        set csvParms(missingValue) [csvDialog.spec.missingValue]
        set csvParms(duplicateKeyValue) [csvDialog.spec.duplicateKeyAction]
        set csvParms(horizontalDimension) [csvDialog.spec.horizontalDimName]
        .wiring.csvImport.delimiters.colWidth set [csvDialog.colWidth]
        wm deiconify .wiring.csvImport
        raise .wiring.csvImport
        csvDialog.requestRedraw
    }  
}

proc csvImportDialogOK {} {
    global csvParms
    csvDialog.spec.horizontalDimName $csvParms(horizontalDimension)
    set filename $csvParms(filename)
    set csvImportFailed [catch "loadVariableFromCSV csvDialog.spec {$filename}" err]
    if $csvImportFailed {
        toplevel .csvImportError
        label .csvImportError.errMsg -text $err
        label .csvImportError.msg -text "Would you like to generate a report?"
        pack .csvImportError.errMsg .csvImportError.msg -side top
        buttonBar .csvImportError "doReport {$filename}"                
    } else {
        reset
        cancelWin .wiring.csvImport
    }
}

proc csvWebImportDialogOK {} {
    global csvParms
    csvDialog.spec.horizontalDimName $csvParms(horizontalDimension)
    set filename $csvParms(filename)
    set csvImportFailed [catch "loadVariableFromCSV csvDialog.spec {$filename}" err]
    if $csvImportFailed {
        toplevel .csvImportError
        label .csvImportError.errMsg -text $err
        label .csvImportError.msg -text "Would you like to generate a report?"
        pack .csvImportError.errMsg .csvImportError.msg -side top
        buttonBar .csvImportError "doReport {$filename}"                
    } else {
        reset
        cancelWin .wiring.csvImport
    }
}

# Allow user to input the full web address of a desired file to opened
proc inputUrl {} {
	set url "http://www.patreon.com/hpcoder"
    toplevel .csvImportURL
    pack [entry .csvImportURL.e -textvariable $url]
    pack [button .csvImportURL.b -text "OK" -command {destroy .csvImportURL}]
    bind .csvImportURL <Return> {.csvImportURL.b invoke}
    focus .csvImportURL.e
    return $url
} 

#proc getFile { url } { 
#   set token [::http::geturl $url]
#   set data [::http::data $token]
#   ::http::cleanup $token          
#   return $data
#}

# Return contents of a file on the web
proc getFile { url } {
  global workDir	
  set f [open $workDir/download.tar wb]
  set tok [http::geturl $url -channel $f -binary 1]
  set data [::http::data $tok]
  close $f
  if {[http::status $tok] eq "ok" && [http::ncode $tok] == 200} {
      puts "Downloaded successfully"
  }
  http::cleanup $tok
  return $data
}

#proc openCSVFromURL {URL} {
#    global tcl_platform
#    if {[tk windowingsystem]=="win32"} {
#        #shellOpen $URL
#        set command [list {*}[auto_execok start] {}]
#        if {[file isdirectory $url]} {
#            # if there is an executable named eg ${url}.exe, avoid opening that instead:
#            set url [file nativename [file join $url .]]             
#    } elseif {$tcl_platform(os)=="Darwin"} {
#        #exec open $URL
#        set command [list open]
#    #} elseif [catch {exec xdg-open $URL &}] {
#    } elseif [catch {set command [list xdg-open]}] {
#        # try a few likely suspects
#        foreach browser {firefox konqueror seamonkey opera} {
#            set browserNotFound [catch {exec $browser $URL &}]
#            if {!$browserNotFound} break
#        }
#        if $browserNotFound {
#            tk_messageBox -detail "Unable to find a working web browser, 
#please consult $URL" -type ok -icon warning
#        }
#    }
#}

proc doReport {inputFname} {
    global workDir
    set fname [tk_getSaveFile -initialfile [file rootname $inputFname]-error-report.csv -initialdir $workDir]
    if [string length $fname] {
        eval csvDialog.reportFromFile {$inputFname} {$fname}
    }
}

        
proc scrollTable v {
    csvDialog.xoffs [expr -$v]
    csvDialog.requestRedraw
}

proc adjustColWidth {w} {
    csvDialog.colWidth $w
    csvDialog.requestRedraw
    return 1
}

proc csvImportButton1 {x y} {
    global csvImportPanX mouseSave movingHeader
    set csvImportPanX [expr [csvDialog.xoffs]-$x]
    set movingHeader [expr [csvDialog.rowOver $y]-4 == [csvDialog.spec.headerRow]]
    set mouseSave "$x $y"
}

proc closeCombo setter {
    eval $setter \[.wiring.csvImport.text.combo get\]
    wm withdraw .wiring.csvImport.text
    csvDialog.requestRedraw
}

proc setupCombo {getter setter title configure X Y} {
    wm title .wiring.csvImport.text $title
    eval .wiring.csvImport.text.combo configure $configure
    .wiring.csvImport.text.combo set $getter
    bind .wiring.csvImport.text.combo <<ComboboxSelected>> "closeCombo $setter"
    bind .wiring.csvImport.text.combo <Return> "closeCombo $setter"
    wm deiconify .wiring.csvImport.text
    wm geometry .wiring.csvImport.text +$X+$Y
    raise .wiring.csvImport.text 
}

proc csvImportButton1Up {x y X Y} {
    global mouseSave csvParms movingHeader
    # combobox used for setting dimension type
    if {![winfo exists .wiring.csvImport.text.combo]} {
        toplevel .wiring.csvImport.text
        ttk::combobox .wiring.csvImport.text.combo -values {"string" "value" "time"} -state readonly
        pack .wiring.csvImport.text.combo
        wm withdraw .wiring.csvImport.text
    }

    set col [csvDialog.columnOver $x]
    set row [csvDialog.rowOver $y]
    if {abs([lindex $mouseSave 0]-$x)==0 && abs([lindex $mouseSave 1]-$y)==0} {
        # mouse click - implement dialog interaction logic
        switch [csvDialog.rowOver $y] {
            0 {csvDialog.spec.toggleDimension $col
                csvDialog.requestRedraw
            }
            1 {if {$col<[csvDialog.spec.dimensions.size]} {
                setupCombo [[csvDialog.spec.dimensions.@elem $col].type] \
                    "csvDialog.spec.dimensions($col).type" \
                    "Dimension type" {-values {"string" "value" "time"} -state readonly} $X $Y
            }}
            2 {if {$col<[csvDialog.spec.dimensions.size]} {
                set units ""
                if {[[csvDialog.spec.dimensions.@elem $col].type]=="time"} {
                    set units "{%Y-%m-%D %Y-%m-%d %H:%M:%S %Y-Q%Q HopkinsDate}"
                }
                setupCombo [[csvDialog.spec.dimensions.@elem $col].units] \
                    "csvDialog.spec.dimensions($col).units" \
                    "Dimension units/format" "-values \"$units\" -state normal" $X $Y
            }}
            3 {if {$col<[csvDialog.spec.dimensions.size]} {
                setupCombo [[csvDialog.spec.dimensionNames.@elem $col]] \
                    "csvDialog.spec.dimensionNames($col)" \
                    "Dimension name" "-values \"[csvDialog.headerForCol $col]\" -state normal" $X $Y
            }}
            default {
                csvDialog.spec.setDataArea [expr $row-4] $col
                csvDialog.requestRedraw
            }
        }
    }
    if {$movingHeader && $row==3} {
        # copy columns headers to dimension names
        set oldHeaderRow [expr [csvDialog.rowOver [lindex $mouseSave 1]]-4]
        csvDialog.copyHeaderRowToDimNames $oldHeaderRow
        csvDialog.spec.headerRow $oldHeaderRow
        csvDialog.flashNameRow 0
        csvDialog.requestRedraw
    }
    set movingHeader 0
}
