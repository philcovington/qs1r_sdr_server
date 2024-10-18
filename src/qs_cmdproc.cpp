/*
Copyright (C) 2010 Philip A Covington
*/

#include "../include/qs_cmdproc.hpp"

CMD::CMD() {
    RW = cmd_error;
    cmd = "";
    svalue.clear();
    slist.clear();
    ivalue = 0;
    dvalue = 0.0;
}

// ------------------------------------------------------------
// QS1RServer Command parser
// ------------------------------------------------------------
CMD CMD ::processCMD(String command) {
    CMD cmd;
    cmd.RW = cmd_error; // error
    cmd.ivalue = 0;
    cmd.dvalue = 0.0;
    cmd.slist.clear();
    cmd.svalue.clear();

    if (command.startsWith(">"))
        cmd.RW = cmd_write;
    else if (command.startsWith("?"))
        cmd.RW = cmd_read;
    else
        return cmd;

    int rwdelimiter = 0;
    int cmddelimiter = 1;
    int valdelimiter = 1;

    cmddelimiter = command.indexOf(" ", rwdelimiter + 1);
    if (cmddelimiter == -1)
        cmddelimiter = command.length();
    valdelimiter = command.length();

    if (cmddelimiter < 1)
        return cmd;
    if (valdelimiter > 1024)
        return cmd;

    cmd.cmd = command.mid(rwdelimiter + 1, cmddelimiter - rwdelimiter - 1);
    cmd.svalue = command.mid(cmddelimiter + 1, valdelimiter - cmddelimiter - 1);

    cmd.slist = cmd.svalue.split(",");

    bool ok = false;

    cmd.ivalue = cmd.svalue.toInt(&ok);

    if (!ok)
        cmd.ivalue = 0;

    cmd.dvalue = cmd.svalue.toDouble(&ok);

    if (!ok)
        cmd.dvalue = 0.0;

    return cmd;
}
