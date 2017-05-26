function  hide_fail (element) {
	if(element.checked)
	{
		if($('#smokediv').length > 0 && $("#smokecb").is(":checked"))
		{

			$("#smokediv tr").hide();
			$("#smokediv td:contains('FAIL')").parent().show();
			$('#smokeattr').show()

		}
		if($("#rnrdiv").length > 0 && $("#rnrcb").is(":checked"))
		{
			$("#rnrdiv tr").hide();
			$("#rnrdiv td:contains('FAIL')").parent().show();
			$('#rnrattr').show()
		}

		if($("#fpsdiv").length > 0 && $("#fpscb").is(":checked"))
		{
			$("#fpsdiv tr").hide();
			$("#fpsdiv td:contains('FAIL')").parent().show();
			$('#fpsattr').show()
		}
		if($("#launchtimediv").length > 0 && $("#launchtimecb").is(":checked"))
		{
			$("#launchtimediv tr").hide();
			$("#launchtimediv td:contains('FAIL')").parent().show();
			$('#launchtimeattr').show()
		}

	}
	else
	{
		if($("#smokecb").is(":checked"))
		{
			$("#smokediv tr").show()
		}
		if($("#rnrcb").is(":checked"))
		{
			$("#rnrdiv tr").show()
		}
		if($("#fpscb").is(":checked"))
		{
			$("#fpsdiv tr").show()
		}
		if($("#launchtimecb").is(":checked"))
		{
			$("#launchtimediv tr").show()
		}
	}
}


function fps(element)
{
	if (element.checked)
	{
		if($('#fpsdiv').length > 0)
		{
			$('#fpsdiv').show();
		}
	}
	else
	{
		if($('#fpsdiv').length > 0)
		{
			{$('#fpsdiv').hide();}
		}
	}
}


function rnr(element)
{
	if (element.checked)
	{
		if($('#rnrdiv').length > 0)
		{
			$('#rnrdiv').show();
		}
	}
	else
	{
		if($('#rnrdiv').length > 0)
		{
			{$('#rnrdiv').hide();}
		}
	}
}


function smoke(element)
{
	if (element.checked)
	{
		if($('#smokediv').length > 0)
		{
			$('#smokediv').show();
		}
	}
	else
	{
		if($('#smokediv').length > 0)
		{
			$('#smokediv').hide();
		}

	}
}

function launchtime(element)
{
	if (element.checked)
	{
		if($('#launchtimediv').length > 0)
		{
			$('#launchtimediv').show();
		}
	}
	else
	{
		if($('#launchtimediv').length > 0)
		{
			$('#launchtimediv').hide();
		}

	}
}
function setcb () {
	if($('#fpsdiv').length == 0)
	{
		$("#fpscb").prop("checked", false);
		$("#fpscb").prop("disabled", true);
	}
	else
	{
		$("#fpscb").prop("checked", true);
		$("#fpscb").prop("disabled", false);
	}

	if($('#launchtimediv').length == 0)
	{
		$("#launchtimecb").prop("checked", false);
		$("#launchtimecb").prop("disabled", true);
	}
	else
	{
		$("#launchtimecb").prop("checked", true);
		$("#launchtimecb").prop("disabled", false);
	}

	if($('#smokediv').length == 0)
	{
		$("#smokecb").prop("checked", false);
		$("#smokecb").prop("disabled", true);
	}
	else
	{
		$("#smokecb").prop("checked", true);
		$("#smokecb").prop("disabled", false);

		var fail_count=0;
		$("#smokediv td:nth-child(14)").each(function  () {
		if ($(this).text() == 'FAIL') {fail_count += 1;};
		})
		var apk_num = $("#smokediv tr").length - 1;
		var pass_apk_num = apk_num - fail_count;
		var pr_num = fail_count/apk_num;
		var pr;

		pr_num = 1 - pr_num;
		if (pr_num == 0)
		{
			pr = "0.00%";
		}
		else if(pr_num == 1)
		{
			pr = "100.00%"
		}
		else
		{
			pr_num = pr_num.toFixed(4);
			pr = pr_num.slice(2,4) + "." + pr_num.slice(4,6) + "%";
		}
		$('#smokepr').after("<b>" + pr + "</b>" + " (" + pass_apk_num + '/' + apk_num + ')')

	}

	if($('#rnrdiv').length == 0)
	{
		$("#rnrcb").prop("checked", false);
		$("#rnrcb").prop("disabled", true);
	}
	else
	{
		$("#rnrcb").prop("checked", true);
		$("#rnrcb").prop("disabled", false);
		var fail_count = 0;
		$("#rnrdiv td:nth-child(15)").each(function  () {
		if ($(this).text() == 'FAIL') {fail_count += 1;};
		})

		var apk_num = $("#rnrdiv tr").length - 1;
		var pass_apk_num = apk_num - fail_count;
		var pr_num = fail_count/apk_num;
		var pr;

		pr_num = 1 - pr_num;
		if (pr_num == 0)
		{
			pr = "0%";
		}
		else if(pr_num == 1)
		{
			pr = "100.00%"
		}
		else
		{
			pr_num = pr_num.toFixed(4);
			pr = pr_num.slice(2,4) + "." + pr_num.slice(4,6) + "%";
		}
		$('#rnrpr').after("<b>" + pr + "</b>" + " (" + pass_apk_num + '/' + apk_num + ')')
	}

	$('#failcb').prop("checked", false);

}

window.onload=setcb