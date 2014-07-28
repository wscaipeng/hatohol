/*
 * Copyright (C) 2013-2014 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
 */

var ActionsView = function(userProfile) {
  //
  // Variables
  //
  var self = this;

  // call the constructor of the super class
  HatoholMonitoringView.apply(this, [userProfile]);

  //
  // main code
  //
  if (userProfile.hasFlag(hatohol.OPPRVLG_CREATE_ACTION))
    $("#add-action-button").show();
  if (userProfile.hasFlag(hatohol.OPPRVLG_DELETE_ACTION) ||
      userProfile.hasFlag(hatohol.OPPRVLG_DELETE_ALL_ACTION)) {
    $("#delete-action-button").show();
  }
  self.startConnection('action', updateCore);

  //
  // Main view
  //
  $("#table").stupidtable();
  $("#table").bind('aftertablesort', function(event, data) {
    var th = $(this).find("th");
    th.find("i.sort").remove();
    var icon = data.direction === "asc" ? "up" : "down";
    th.eq(data.column).append("<i class='sort glyphicon glyphicon-arrow-" + icon +"'></i>");
  });

  $("#add-action-button").click(function() {
    new HatoholAddActionDialog(addSucceededCb);
  });

  $("#delete-action-button").click(function() {
    var msg = gettext("Do you delete the selected items ?");
    hatoholNoYesMsgBox(msg, deleteActions);
  });

  function addSucceededCb() {
    self.startConnection('action', updateCore);
  }

  //
  // Commonly used functions from a dialog.
  //
  function parseResult(data) {
    var msg;
    var malformed = false;
    if (data.result == undefined)
      malformed = true;
    if (!malformed && !data.result && data.message == undefined)
      malformed = true;
    if (malformed) {
      msg = "The returned content is malformed: " +
        "Not found 'result' or 'message'.\n" + JSON.stringify(data);
      hatoholErrorMsgBox(msg);
      return false;
    }
    if (!data.result) {
      msg = "Failed:\n" + data.message;
      hatoholErrorMsgBox(msg);
      return false;
    }

    if (data.id == undefined) {
      msg = "The returned content is malformed: " +
        "'result' is true, however, 'id' is missing.\n" +
        JSON.stringify(data);
      hatoholErrorMsgBox(msg);
      return false;
    }
    return true;
  }

  //
  // delete-action dialog
  //
  function deleteActions() {
    $(this).dialog("close");
    var checkbox = $(".selectcheckbox");
    var deletedIdArray = {count:0, total:0, errors:0};
    for (var i = 0; i < checkbox.length; i++) {
      if (!checkbox[i].checked)
        continue;
      var actionId = checkbox[i].getAttribute("actionId");
      deletedIdArray[actionId] = true;
      deletedIdArray.count++;
      deletedIdArray.total++;
      deleteOneAction(actionId, deletedIdArray);
    }
    hatoholInfoMsgBox(gettext("Deleting..."));
  }

  function deleteOneAction(id, deletedIdArray) {
    new HatoholConnector({
      url: '/action/' + id,
      request: "DELETE",
      context: deletedIdArray,
      replyCallback: function(data, parser, context) {
        parseDeleteActionResult(data, context);
      },
      connectErrorCallback: function(XMLHttpRequest,
                                     textStatus, errorThrown) {
        var errorMsg = "Error: " + XMLHttpRequest.status + ": " +
          XMLHttpRequest.statusText;
        hatoholErrorMsgBox(errorMsg);
        deletedIdArray.errors++;
      },
      completionCallback: function(context) {
        compleOneDelAction(context);
      },
    });
  }

  function parseDeleteActionResult(data, deletedIdArray) {
    if (!parseResult(data))
      return;
    if (!(data.id in deletedIdArray)) {
      alert("Fatal Error: You should reload page.\nID: " + data.id +
            " is not in deletedIdArray: " + deletedIdArray);
      deletedIdArray.errors++;
      return;
    }
    delete deletedIdArray[data.id];
  }

  function compleOneDelAction(deletedIdArray) {
    deletedIdArray.count--;
    var completed = deletedIdArray.total - deletedIdArray.count;
    hatoholErrorMsgBox(gettext("Deleting...") + " " + completed +
                       " / " + deletedIdArray.total);
    if (deletedIdArray.count > 0)
      return;

    // close dialogs
    hatoholInfoMsgBox(gettext("Completed. (Number of errors: ") +
                      deletedIdArray.errors + ")");

    // update the main view
    self.startConnection('action', updateCore);
  }

  //
  // Functions for make the main view.
  //
  function makeNamelessServerLabel(serverId) {
    return "(ID:" + serverId + ")";
  }

  function makeNamelessHostgroupLabel(serverId, hostgroupId) {
    return "(S" + serverId + "-G" + hostgroupId + ")";
  }

  function makeNamelessHostLabel(serverId, hostId) {
    return "(S" + serverId + "-H" + hostId + ")";
  }

  function makeNamelessTriggerLabel(triggerId) {
    return "(T" + triggerId + ")";
  }

  function makeTypeLabel(type) {
    switch(type) {
    case ACTION_COMMAND:
      return gettext("COMMAND");
    case ACTION_RESIDENT:
      return gettext("RESIDENT");
    default:
      return "INVALID: " + type;
    }
  }

  function makeSeverityCompTypeLabel(compType) {
    switch(compType) {
    case CMP_EQ:
      return "=";
    case CMP_EQ_GT:
      return ">=";
    default:
      return "INVALID: " + compType;
    }
  }

  //
  // parser of received json data
  //
  function getServerNameFromAction(actionsPkt, actionDef) {
    var serverId = actionDef["serverId"];
    if (!serverId)
      return "ANY";
    var server = actionsPkt["servers"][serverId];
    if (!server)
      return makeNamelessServerLabel(serverId);
    var serverName = actionsPkt["servers"][serverId]["name"];
    if (!serverName)
      return makeNamelessServerLabel(serverId);
    return serverName;
  }

  function getHostgroupNameFromAction(actionsPkt, actionDef) {
    var hostgroupId = actionDef["hostgroupId"];
    if (!hostgroupId)
      return "ANY";
    var serverId = actionDef["serverId"];
    if (!serverId)
      return makeNamelessHostgroupLabel(serverId, hostgroupId);
    var server = actionsPkt["servers"][serverId];
    if (!server)
      return "ANY";
    var hostgroupArray = server["groups"];
    if (!hostgroupArray)
      return makeNamelessHostgroupLabel(serverId, hostgroupId);
    var hostgroup = hostgroupArray[hostgroupId];
    if (!hostgroup)
      return makeNamelessHostgroupLabel(serverId, hostgroupId);
    var hostgroupName = hostgroup["name"];
    if (!hostgroupName)
      return makeNamelessHostgroupLabel(serverId, hostgroupId);
    return hostgroupName;
  }

  function getHostNameFromAction(actionsPkt, actionDef) {
    var hostId = actionDef["hostId"];
    if (!hostId)
      return "ANY";
    var serverId = actionDef["serverId"];
    if (!serverId)
      return makeNamelessHostLabel(serverId, hostId);
    var server = actionsPkt["servers"][serverId];
    if (!server)
      return "ANY";
    var hostArray = server["hosts"];
    if (!hostArray)
      return makeNamelessHostLabel(serverId, hostId);
    var host = hostArray[hostId];
    if (!host)
      return makeNamelessHostLabel(serverId, hostId);
    var hostName = host["name"];
    if (!hostName)
      return makeNamelessHostLabel(serverId, hostId);
    return hostName;
  }

  function getTriggerBriefFromAction(actionsPkt, actionDef) {
    var triggerId = actionDef["triggerId"];
    if (!triggerId)
      return "ANY";
    var serverId = actionDef["serverId"];
    if (!serverId)
      return makeNamelessTriggerLabel(triggerId);
    var server = actionsPkt["servers"][serverId];
    if (!server)
      return "ANY";
    var triggerArray = server["triggers"];
    if (!triggerArray)
      return makeNamelessTriggerLabel(triggerId);
    var trigger = triggerArray[triggerId];
    if (!trigger)
      return makeNamelessTriggerLabel(triggerId);
    var triggerBrief = trigger["brief"];
    if (!triggerBrief)
      return makeNamelessTriggerLabel(triggerId);
    return triggerBrief;
  }

  //
  // callback function from the base template
  //
  function drawTableBody(actionsPkt) {
    var x;
    var klass, server, host;

    var s = "";
    for (x = 0; x < actionsPkt["actions"].length; ++x) {
      var actionDef = actionsPkt["actions"][x];
      s += "<tr>";
      s += "<td class='delete-selector' style='display:none;'>";
      s += "<input type='checkbox' class='selectcheckbox' " +
        "actionId='" + escapeHTML(actionDef.actionId) + "'></td>";
      s += "<td>" + escapeHTML(actionDef.actionId) + "</td>";

      var serverName = getServerNameFromAction(actionsPkt, actionDef);
      s += "<td>" + escapeHTML(serverName) + "</td>";

      var hostName = getHostNameFromAction(actionsPkt, actionDef);
      s += "<td>" + escapeHTML(hostName)   + "</td>";

      var hostgroupName = getHostgroupNameFromAction(actionsPkt, actionDef);
      s += "<td>" + escapeHTML(hostgroupName) + "</td>";

      var triggerBrief = getTriggerBriefFromAction(actionsPkt, actionDef);
      s += "<td>" + escapeHTML(triggerBrief) + "</td>";

      var triggerStatus = actionDef.triggerStatus;
      var triggerStatusLabel = "ANY";
      if (triggerStatus != undefined)
        triggerStatusLabel = makeTriggerStatusLabel(triggerStatus);
      s += "<td>" + triggerStatusLabel + "</td>";

      var triggerSeverity = actionDef.triggerSeverity;
      var severityLabel = "ANY";
      if (triggerSeverity != undefined)
        severityLabel = makeSeverityLabel(triggerSeverity);

      var severityCompType = actionDef.triggerSeverityComparatorType;
      var severityCompLabel = "";
      if (triggerSeverity != undefined)
        severityCompLabel = makeSeverityCompTypeLabel(severityCompType);

      s += "<td>" + severityCompLabel + " " + severityLabel + "</td>";

      var type = actionDef.type;
      var typeLabel = makeTypeLabel(type);
      s += "<td>" + typeLabel + "</td>";

      var workingDir = actionDef.workingDirectory;
      if (!workingDir)
        workingDir = "N/A";
      s += "<td>" + escapeHTML(workingDir) + "</td>";

      var command = actionDef.command;
      s += "<td>" + escapeHTML(command) + "</td>";

      var timeout = actionDef.timeout;
      if (timeout == 0)
        timeout = gettext("No limit");
      s += "<td>" + escapeHTML(timeout) + "</td>";

      s += "</tr>";
    }

    return s;
  }

  function updateCore(rawData) {
    $("#table tbody").empty();
    $("#table tbody").append(drawTableBody(rawData));
    self.setupCheckboxForDelete($("#delete-action-button"));
  }
};

ActionsView.prototype = Object.create(HatoholMonitoringView.prototype);
ActionsView.prototype.constructor = ActionsView;
