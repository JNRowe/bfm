
/*
 *
 * gkrellm-bfm.c
 * Pigeon(pigeon@pigeond.net)
 *
 * http://pigeond.net/bfm/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *     02111-1307, USA.
 *
 */

#ifdef GKRELLM2
#include <gkrellm2/gkrellm.h>
#else
#include <gkrellm/gkrellm.h>
#endif
#include <libgen.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>

#define PLUGIN_VERSION	"0.6.1"

#define PLUGIN_NAME	"gkrellm-bfm"
#define PLUGIN_DESC	"bubblefishymon gkrellm port"
#define PLUGIN_URL	"http://www.jnrowe.uklinux.net.net/projects/bfm/"
#define PLUGIN_STYLE	PLUGIN_NAME
#define PLUGIN_KEYWORD	PLUGIN_NAME

#define DEBUG 0


#define CHART_W 56
#define CHART_H 56

#ifdef GKRELLM2
# define Monitor     GkrellmMonitor
# define Chart       GkrellmChart
# define ChartConfig GkrellmChartconfig

# define gkrellm_create_tab        gkrellm_gtk_notebook_page
# define gkrellm_create_framed_tab gkrellm_gtk_framed_notebook_page
# define gkrellm_scrolled_text     gkrellm_gtk_scrolled_text_view
# define gkrellm_add_info_text     gkrellm_gtk_text_view_append_strings

# define init_plugin gkrellm_init_plugin
#endif

static gint style_id;
static char *prog = NULL;

static Monitor *mon = NULL;
static Chart *chart = NULL;
static ChartConfig *chart_config = NULL;



/* From the actual bfm */
int bfm_main();
void gkrellm_update(GtkWidget *widget, GdkDrawable *drawable, int start_x, int proximity);
void prepare_sprites(void);
extern int cpu_enabled;
extern int duck_enabled;
extern int fish_enabled;
extern int fish_traffic;
extern int time_enabled;
extern int memscreen_enabled;

int proximity = 0;

/* Options stuffs */
GtkWidget *prog_entry = NULL;
GtkWidget *cpu_check = NULL;
GtkWidget *mem_check = NULL;
GtkWidget *duck_check = NULL;
GtkWidget *fish_check = NULL;
GtkWidget *clock_check = NULL;
GtkWidget *fish_traffic_check = NULL;

static void
update_plugin(void)
{
	GdkEventExpose event;
	gint ret_val;
	gtk_signal_emit_by_name(GTK_OBJECT(chart->drawing_area), "expose_event", &event, &ret_val);

}


static gint
chart_expose_event(GtkWidget *widget, GdkEventExpose *ev)
{
	int x;
	x = (gkrellm_chart_width() - CHART_W) / 2;
	gkrellm_update(widget, chart->pixmap, x, proximity);
	gkrellm_draw_chart_to_screen(chart);
	return TRUE;
}


static gint
button_release_event(GtkWidget *widget, GdkEventButton *ev, gpointer data)
{
	switch(ev->button)
	{
		case 1:
		case 2:
			if(prog)
			{
				gchar *cmd;
				cmd = g_strdup_printf("%s &", prog);
				system(cmd);
				g_free(cmd);
			}
			break;

		case 3:
			gkrellm_open_config_window(mon);
			break;

		default:
			break;
	}

	return TRUE;
}

static gint
enter_notify_event(GtkWidget *widget, GdkEventMotion *ev, gpointer data)
{
	proximity = 1;
	return TRUE;
}

static gint
leave_notify_event(GtkWidget *widget, GdkEventMotion *ev, gpointer data)
{
	proximity = 0;
	return TRUE;
}

static void
create_plugin(GtkWidget *vbox, gint first_create)
{
	if(first_create)
	{
		chart = gkrellm_chart_new0();
	}

	gkrellm_set_chart_height_default(chart, CHART_H);
	gkrellm_chart_create(vbox, mon, chart, &chart_config);

	if(first_create)
	{
		bfm_main();
		gtk_signal_connect(GTK_OBJECT(chart->drawing_area),
				"expose_event",
				(GtkSignalFunc) chart_expose_event,
				NULL);
		gtk_signal_connect(GTK_OBJECT(chart->drawing_area),
				"button_release_event", GTK_SIGNAL_FUNC(button_release_event),
				NULL);
		gtk_signal_connect(GTK_OBJECT(chart->drawing_area),
				"enter_notify_event", GTK_SIGNAL_FUNC(enter_notify_event),
				NULL);
		gtk_signal_connect(GTK_OBJECT(chart->drawing_area),
				"leave_notify_event", GTK_SIGNAL_FUNC(leave_notify_event),
				NULL);
	}

}


static void
option_toggled_cb(GtkToggleButton *button, gpointer user_data)
{
	gboolean active = gtk_toggle_button_get_active(button);
	GtkWidget *togglebutton = GTK_WIDGET(button);

	if(togglebutton == cpu_check)
	{
		cpu_enabled = active;
	}
	else if(togglebutton == mem_check)
	{
		memscreen_enabled = active;
	}
	else if(togglebutton == duck_check)
	{
		duck_enabled = active;
	}
	else if(togglebutton == fish_check)
	{
		fish_enabled = active;
		if(fish_enabled)
		{
			gtk_widget_set_sensitive(fish_traffic_check, TRUE);
		}
		else
		{
			gtk_widget_set_sensitive(fish_traffic_check, FALSE);
		}
	}
	else if(togglebutton == clock_check)
	{
		time_enabled = active;
	}
	else if(togglebutton == fish_traffic_check)
	{
	    fish_traffic = active;
		/* Call this to reinitialize the fishes */
	    prepare_sprites();
	}

}

void
setup_toggle_buttons(void)
{
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cpu_check), cpu_enabled);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mem_check), memscreen_enabled);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(duck_check), duck_enabled);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fish_check), fish_enabled);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(clock_check), time_enabled);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fish_traffic_check), fish_traffic);
}

static void
create_plugin_tab(GtkWidget *tab_vbox)
{
	GtkWidget *tabs = NULL;

	GtkWidget *options_tab = NULL;

	GtkWidget *info_tab = NULL;
	GtkWidget *info = NULL;

	GtkWidget *about_tab = NULL;
	GtkWidget *about = NULL;

	GtkWidget *main_box;
	GtkWidget *prog_box;
	GtkWidget *prog_label;
	GtkWidget *row1;
	GtkWidget *cpu_box;
	GtkWidget *mem_box;
	GtkWidget *row2;
	GtkWidget *duck_box;
	GtkWidget *fish_box;
	GtkWidget *row3;
	GtkWidget *clock_box;
	GtkWidget *fish_traffic_box;

	static gchar *info_text[] =
	{
		"<b>" PLUGIN_NAME " " PLUGIN_VERSION "\n\n",
		"bubblefishymon gkrellm port\n",
		"Pigeon(pigeon@pigeond.net)\n",
		PLUGIN_URL "\n\n",
		"Original bubblemon by Johan\n",
		"http://www.student.nada.kth.se/~d92-jwa/code/\n\n",
		"More hacks by Timecop\n",
		"http://www.ne.jp/asahi/linux/timecop/\n\n",
		"And more hacks by James Rowe\n\n",
		"<i>Usage\n\n",
		"Nice monitor with:\n",
		"- Water level representing the memory usage...\n",
		"- Water color representing swap status...\n",
		"- Bubbles representing CPU usage...\n",
		"- Fish representing network traffic...\n",
		"  (Fish swiming from left to right represents outgoing traffic,\n",
		"   fish swiming from right to left represents incoming traffic)\n",
		"- Cute little duck swimming...\n",
		"- Clock hands representing time (obviously)...\n",
		"- Click and it will run a command for you (requested by Nick =)\n\n",
		"<i>Notes\n\n",
		"- Currently Gkrellm updates at most 10 times a second, and so\n",
		"  BFM updates is a bit jerky still.\n",
		"\n\n",
	};

	static gchar *about_text =
	_(
	  "BubbleFishyMon " PLUGIN_VERSION "\n"
	  PLUGIN_NAME " - " PLUGIN_DESC "\n\n"
	  "Copyright (C) 2001 Pigeon\n"
	  "pigeon@pigeond.net\n"
	  PLUGIN_URL "\n\n"
	  "Released under the GNU Public Licence"
	);

	tabs = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(tabs), GTK_POS_TOP);
	gtk_box_pack_start(GTK_BOX(tab_vbox), tabs, TRUE, TRUE, 0);

	/* Options tab */
	options_tab = gkrellm_create_tab(tabs, _("Options"));

	main_box = gtk_vbox_new (FALSE, 0);
	gtk_widget_set_name (main_box, "main_box");
	gtk_widget_ref (main_box);
	gtk_object_set_data_full (GTK_OBJECT (options_tab), "main_box", main_box,
							(GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (main_box);
	gtk_container_add (GTK_CONTAINER (options_tab), main_box);

	prog_box = gtk_hbox_new (FALSE, 0);
	gtk_widget_set_name (prog_box, "prog_box");
	gtk_widget_ref (prog_box);
	gtk_object_set_data_full (GTK_OBJECT (options_tab), "prog_box", prog_box,
							(GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (prog_box);
	gtk_box_pack_start (GTK_BOX (main_box), prog_box, TRUE, TRUE, 0);

	prog_label = gtk_label_new (_("Program to execute when clicked: "));
	gtk_widget_set_name (prog_label, "prog_label");
	gtk_widget_ref (prog_label);
	gtk_object_set_data_full (GTK_OBJECT (options_tab), "prog_label", prog_label,
							(GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (prog_label);
	gtk_box_pack_start (GTK_BOX (prog_box), prog_label, FALSE, FALSE, 0);

	prog_entry = gtk_entry_new ();
	if(prog)
	{
		gtk_entry_set_text(GTK_ENTRY(prog_entry), prog);
	}
	gtk_widget_set_name (prog_entry, "prog_entry");
	gtk_widget_ref (prog_entry);
	gtk_object_set_data_full (GTK_OBJECT (options_tab), "prog_entry", prog_entry,
							(GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (prog_entry);
	gtk_box_pack_start (GTK_BOX (prog_box), prog_entry, TRUE, TRUE, 0);

	row1 = gtk_hbox_new (FALSE, 0);
	gtk_widget_set_name (row1, "row1");
	gtk_widget_ref (row1);
	gtk_object_set_data_full (GTK_OBJECT (options_tab), "row1", row1,
							(GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (row1);
	gtk_box_pack_start (GTK_BOX (main_box), row1, TRUE, TRUE, 0);

	cpu_box = gtk_hbox_new (FALSE, 0);
	gtk_widget_set_name (cpu_box, "cpu_box");
	gtk_widget_ref (cpu_box);
	gtk_object_set_data_full (GTK_OBJECT (options_tab), "cpu_box", cpu_box,
							(GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (cpu_box);
	gtk_box_pack_start (GTK_BOX (row1), cpu_box, FALSE, TRUE, 0);

	cpu_check = gtk_check_button_new_with_label (_("CPU"));
	gtk_widget_set_name (cpu_check, "cpu_check");
	gtk_widget_ref (cpu_check);
	gtk_object_set_data_full (GTK_OBJECT (options_tab), "cpu_check", cpu_check,
							(GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (cpu_check);
	gtk_box_pack_start (GTK_BOX (cpu_box), cpu_check, FALSE, TRUE, 0);
	gtk_widget_set_usize (cpu_check, 220, -2);

	mem_box = gtk_hbox_new (FALSE, 0);
	gtk_widget_set_name (mem_box, "mem_box");
	gtk_widget_ref (mem_box);
	gtk_object_set_data_full (GTK_OBJECT (options_tab), "mem_box", mem_box,
							(GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (mem_box);
	gtk_box_pack_start (GTK_BOX (row1), mem_box, TRUE, TRUE, 0);

	mem_check = gtk_check_button_new_with_label (_("Memory"));
	gtk_widget_set_name (mem_check, "mem_check");
	gtk_widget_ref (mem_check);
	gtk_object_set_data_full (GTK_OBJECT (options_tab), "mem_check", mem_check,
							(GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (mem_check);
	gtk_box_pack_start (GTK_BOX (mem_box), mem_check, TRUE, TRUE, 0);

	row2 = gtk_hbox_new (FALSE, 0);
	gtk_widget_set_name (row2, "row2");
	gtk_widget_ref (row2);
	gtk_object_set_data_full (GTK_OBJECT (options_tab), "row2", row2,
							(GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (row2);
	gtk_box_pack_start (GTK_BOX (main_box), row2, TRUE, TRUE, 0);

	duck_box = gtk_hbox_new (FALSE, 0);
	gtk_widget_set_name (duck_box, "duck_box");
	gtk_widget_ref (duck_box);
	gtk_object_set_data_full (GTK_OBJECT (options_tab), "duck_box", duck_box,
							(GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (duck_box);
	gtk_box_pack_start (GTK_BOX (row2), duck_box, FALSE, TRUE, 0);

	duck_check = gtk_check_button_new_with_label (_("Duck"));
	gtk_widget_set_name (duck_check, "duck_check");
	gtk_widget_ref (duck_check);
	gtk_object_set_data_full (GTK_OBJECT (options_tab), "duck_check", duck_check,
							(GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (duck_check);
	gtk_box_pack_start (GTK_BOX (duck_box), duck_check, FALSE, TRUE, 0);
	gtk_widget_set_usize (duck_check, 220, -2);

	fish_box = gtk_hbox_new (FALSE, 0);
	gtk_widget_set_name (fish_box, "fish_box");
	gtk_widget_ref (fish_box);
	gtk_object_set_data_full (GTK_OBJECT (options_tab), "fish_box", fish_box,
							(GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (fish_box);
	gtk_box_pack_start (GTK_BOX (row2), fish_box, TRUE, TRUE, 0);

	fish_check = gtk_check_button_new_with_label (_("Fish"));
	gtk_widget_set_name (fish_check, "fish_check");
	gtk_widget_ref (fish_check);
	gtk_object_set_data_full (GTK_OBJECT (options_tab), "fish_check", fish_check,
							(GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (fish_check);
	gtk_box_pack_start (GTK_BOX (fish_box), fish_check, TRUE, TRUE, 0);

	row3 = gtk_hbox_new (FALSE, 0);
	gtk_widget_set_name (row3, "row3");
	gtk_widget_ref (row3);
	gtk_object_set_data_full (GTK_OBJECT (options_tab), "row3", row3,
							(GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (row3);
	gtk_box_pack_start (GTK_BOX (main_box), row3, TRUE, TRUE, 0);

	clock_box = gtk_hbox_new (FALSE, 0);
	gtk_widget_set_name (clock_box, "clock_box");
	gtk_widget_ref (clock_box);
	gtk_object_set_data_full (GTK_OBJECT (options_tab), "clock_box", clock_box,
							(GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (clock_box);
	gtk_box_pack_start (GTK_BOX (row3), clock_box, FALSE, TRUE, 0);

	clock_check = gtk_check_button_new_with_label (_("Show clock"));
	gtk_widget_set_name (clock_check, "clock_check");
	gtk_widget_ref (clock_check);
	gtk_object_set_data_full (GTK_OBJECT (options_tab), "clock_check", clock_check,
							(GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (clock_check);
	gtk_box_pack_start (GTK_BOX (clock_box), clock_check, FALSE, TRUE, 0);
	gtk_widget_set_usize (clock_check, 220, -2);

	fish_traffic_box = gtk_hbox_new (FALSE, 0);
	gtk_widget_set_name (fish_traffic_box, "fish_traffic_box");
	gtk_widget_ref (fish_traffic_box);
	gtk_object_set_data_full (GTK_OBJECT (options_tab), "fish_traffic_box", fish_traffic_box,
							(GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (fish_traffic_box);
	gtk_box_pack_start (GTK_BOX (row3), fish_traffic_box, TRUE, TRUE, 0);

	fish_traffic_check = gtk_check_button_new_with_label (_("Fish represents network traffic"));
	gtk_widget_set_name (fish_traffic_check, "fish_traffic_check");
	gtk_widget_ref (fish_traffic_check);
	gtk_object_set_data_full (GTK_OBJECT (options_tab), "fish_traffic_check", fish_traffic_check,
							(GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (fish_traffic_check);
	gtk_box_pack_start (GTK_BOX (fish_traffic_box), fish_traffic_check, TRUE, TRUE, 0);

	setup_toggle_buttons();

	gtk_signal_connect(GTK_OBJECT(cpu_check), "toggled", GTK_SIGNAL_FUNC(option_toggled_cb), NULL);
	gtk_signal_connect(GTK_OBJECT(mem_check), "toggled", GTK_SIGNAL_FUNC(option_toggled_cb), NULL);
	gtk_signal_connect(GTK_OBJECT(duck_check), "toggled", GTK_SIGNAL_FUNC(option_toggled_cb), NULL);
	gtk_signal_connect(GTK_OBJECT(fish_check), "toggled", GTK_SIGNAL_FUNC(option_toggled_cb), NULL);
	gtk_signal_connect(GTK_OBJECT(clock_check), "toggled", GTK_SIGNAL_FUNC(option_toggled_cb), NULL);
	gtk_signal_connect(GTK_OBJECT(fish_traffic_check), "toggled", GTK_SIGNAL_FUNC(option_toggled_cb), NULL);


	/* Info tab */
	info_tab = gkrellm_create_framed_tab(tabs, _("Info"));
	info = gkrellm_scrolled_text(info_tab, NULL, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gkrellm_add_info_text(info, info_text, sizeof(info_text) / sizeof(gchar *));


	/* About tab */
	about_tab = gkrellm_create_tab(tabs, _("About"));
	about = gtk_label_new(about_text);
	gtk_box_pack_start(GTK_BOX(about_tab), about, TRUE, TRUE, 0);

}

static void
apply_plugin_config(void)
{
	if(prog)
	{
		g_free(prog);
	}
	prog = g_strdup(gtk_editable_get_chars(GTK_EDITABLE(prog_entry), 0, -1));
}

static void
save_plugin_config(FILE *f)
{
	if(prog)
	{
		fprintf(f, "%s prog %s\n", PLUGIN_KEYWORD, prog);
	}
	fprintf(f, "%s options %d.%d.%d.%d.%d.%d\n", PLUGIN_KEYWORD,
			cpu_enabled,
			duck_enabled,
			memscreen_enabled,
			fish_enabled,
			fish_traffic,
			time_enabled);
}

static void
load_plugin_config(gchar *config_line)
{
	gchar *config_item, *value;

	config_item = strtok(config_line, " \n");

	if(!config_item)
		return;

	value = strtok(NULL, "\n");

	if(!strcmp(config_item, "prog"))
	{
		g_free(prog);
		prog = g_strdup(value);
	}
	else if(!strcmp(config_item, "options"))
	{
		sscanf(value, "%d.%d.%d.%d.%d.%d",
				&cpu_enabled,
				&duck_enabled,
				&memscreen_enabled,
				&fish_enabled,
				&fish_traffic,
				&time_enabled);
	}

}


static Monitor bfm_mon =
{
	PLUGIN_NAME,         /* Name, for config tab.                    */
	0,                   /* Id,  0 if a plugin                       */
	create_plugin,       /* The create_plugin() function             */
	update_plugin,       /* The update_plugin() function             */
	create_plugin_tab,   /* The create_plugin_tab() config function  */
	apply_plugin_config, /* The apply_plugin_config() function       */
	
	save_plugin_config,  /* The save_plugin_config() function        */
	load_plugin_config,  /* The load_plugin_config() function        */
	PLUGIN_KEYWORD,      /* config keyword                           */
	
	NULL,                /* Undefined 2                              */
	NULL,                /* Undefined 1                              */
	NULL,                /* private                                  */
	
	MON_CPU,             /* Insert plugin before this monitor.       */
	NULL,                /* Handle if a plugin, filled in by GKrellM */
	NULL                 /* path if a plugin, filled in by GKrellM   */
};


Monitor *
init_plugin(void)
{
	style_id = gkrellm_add_meter_style(&bfm_mon, PLUGIN_STYLE);
	return (mon = &bfm_mon);
}



