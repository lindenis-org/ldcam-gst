#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <gst/gst.h>
#include <glib.h>

#define P_SUF_NAME	".jpg"
#define V_SUF_NAME	".ts"

#define VIDEO_AUDIO

static int flag = 0;

typedef struct
{
	int mode;
	int display;
	int width;
	int height;
	int time;
	char dev[8];
	char path[256];

	int err_status;

	GMainLoop *loop;
	GstElement *pipeline;
	GstElement *camera_src;
	GstElement *src_filter;
	GstElement *video_cov;
	GstElement *video_covf;
	GstElement *sunxi_sink;
	
	GstElement *jpeg_enc;
	GstElement *fake_sink;
	
	GstElement *file_enc;
	GstElement *file_mux;
	GstElement *file_sink;
	GstElement *alsa_src;
	GstElement *alsa_enc;
	GstElement *alsa_filter;
	
	GstElement *tee;
	GstElement *screen_queue;
	GstElement *fake_queue;
	GstElement *file_queue;
	GstElement *alsa_queue;

	int fake_sink_id;
}LDCAM_STATE;

static char* commd_line[]=
{
	"-?:	get help information of this app, same as --help",
	"-c:	capture a photo of camera",
	"-v:	record the video of camera",
	"-w:	set  photo/video width of the file",
	"-h:	set  photo/video height of the file",
	"-o:	set  photo/video output path of the file",
	"-t:	set  photo shot wait time, or video record time",
	"-d:	set  device port of camera",
	"--nodisplay	disable screen display",
};
static void display_cmd_parameters(const char *app_name)
{
	fprintf(stdout, "**************%s need commd as follow:************** \n", app_name);
	for(int i=0; i<sizeof(commd_line)/sizeof(commd_line[0]); i++)
		fprintf(stdout, "	%s \n", commd_line[i]);
	return;
}

int default_state(LDCAM_STATE *state)
{
	state->mode = 0;
	state->display = 1;
	state->width = 1280;
	state->height = 720;
	state->time = 0;
	state->err_status = 0;
	strcpy(state->dev, "0");
	memset(state->path, 0, sizeof(state->path));

	return 0;
}

int paser_cmdline(int argc, const char **argv, LDCAM_STATE *state)
{
	int valid = 1;
	int set_value = 1;
	int i = 0;
	int tmp_value = 0;

	for(i=1; i<argc && valid; i++)
	{
		if(!argv[i])
			continue;
		if(argv[i][0] != '-')
		{
			valid = 0;
			continue;
		}
		if(strcmp("--nodisplay", argv[i]) == 0)
		{
			state->display = 0;
			continue;
		}
		tmp_value = 0;
		valid = 1;
		switch(argv[i][1])
		{
			case 'c':
				if (sscanf("0", "%u", &tmp_value) != 1)
					valid = 0;
				else{
					state->mode = tmp_value;
				}
				break;
			case 'v':
				if (sscanf("1", "%u", &tmp_value) != 1)
					valid = 0;
				else{
					state->mode = tmp_value;
				}
				break;
			case 'd':
				if(!argv[i + 1])
				{
					set_value = 0;
					valid = 0;
					break;
				}
				if (sscanf(argv[i + 1], "%s", state->dev) != 1)
					valid = 0;
				else{
					//state->dev = tmp_value;
					i++;
				}
				break;
			case 'w':
				if(!argv[i + 1])
				{
					set_value = 0;
					valid = 0;
					break;
				}
				if (sscanf(argv[i + 1], "%u", &tmp_value) != 1)
					valid = 0;
				else{
					state->width = tmp_value;
					i++;
				}
				break;
			case 'h':
				if(!argv[i + 1])
				{
					set_value = 0;
					valid = 0;
					break;
				}
				if (sscanf(argv[i + 1], "%u", &tmp_value) != 1)
					valid = 0;
				else{
					state->height = tmp_value;
					i++;
				}
				break;
			case 't':
				if(!argv[i + 1])
				{
					set_value = 0;
					valid = 0;
					break;
				}
				if (sscanf(argv[i + 1], "%u", &tmp_value) != 1)
					valid = 0;
				else{
					state->time = tmp_value;
					i++;
				}
				break;
			case 'o':
				if(!argv[i + 1])
				{
					set_value = 0;
					valid = 0;
					break;
				}
				if (sscanf(argv[i + 1], "%s", state->path) != 1)
					valid = 0;
				else{
					char tmp[256];
					char home_path[256];
					//strcpy(home_path, getenv("HOME"));
					getcwd(home_path, sizeof(home_path));
					strcat(home_path, "/");
				
					int len = strlen(state->path);
					if(state->mode == 0)
					{
						int p_suf_len = strlen(P_SUF_NAME);
						if (len > p_suf_len)
						{
				       			strncpy(tmp, state->path+(len-p_suf_len), p_suf_len);
							tmp[p_suf_len] = '\0';
							if(strcmp(tmp, P_SUF_NAME) != 0)
							{
								strcat(state->path, P_SUF_NAME);
							}
						}
						else{
							strcat(state->path, P_SUF_NAME);
						}

					}
					else{
						int v_suf_len = strlen(V_SUF_NAME);
						if (len > v_suf_len)
						{
				       			strncpy(tmp, state->path+(len-v_suf_len), v_suf_len);
							tmp[v_suf_len] = '\0';
							if(strcmp(tmp, V_SUF_NAME) != 0)
							{
								strcat(state->path, V_SUF_NAME);
							}
						}
						else{
							strcat(state->path, V_SUF_NAME);
						}	
					}

					strcat(home_path, state->path);
					strcpy(state->path, home_path);
					fprintf(stdout, "state path is : %s\n", state->path);
					i++;
				}
				break;
			default:
				valid = 0;
				break;
		}
	}

	if(!valid)
	{
		if(!set_value)
			fprintf(stderr, "Invalid value for : %s\n", argv[i-1]);
		else
			fprintf(stderr, "Invalid commdline option : %s\n", argv[i-1]);
		return -1;
	}
	if(strlen(state->path)==0)
	{
		fprintf(stderr, "Please set the output file's name\n");
		return -1;
	}
	return 0;
}

static gboolean buffer_probe_callback(GstElement *fake_sink, GstBuffer *buffer, GstPad *pad, LDCAM_STATE *state)
{
	fprintf(stdout, "buffer_probe_callback\n");

	GstMapInfo map;
	gst_buffer_map(buffer,&map,GST_MAP_READ);

	fprintf(stdout, "state path is : %s\n", state->path);
	FILE *fp = fopen(state->path, "wb");
	if(!fp)
	{
		fprintf(stdout, "create file fail\n");
		g_signal_handler_disconnect(G_OBJECT(fake_sink), state->fake_sink_id);
		g_main_loop_quit (state->loop);
		gst_buffer_unmap (buffer,&map);
		return FALSE;
	}

	fwrite(map.data, 1, map.size, fp);
	fclose(fp);

	g_signal_handler_disconnect(G_OBJECT(fake_sink), state->fake_sink_id);
	g_main_loop_quit (state->loop);
	gst_buffer_unmap (buffer,&map);
	return TRUE;
}

static void exit_handler(int sig)
{
	flag = 1;
}

void *photo_shot(void *arg)
{
	fprintf(stdout, "photo_shot_callback\n");
	LDCAM_STATE *ld_state = (LDCAM_STATE *)arg;
	signal(SIGINT, exit_handler);
	int i = ld_state->time;
	if(i <= 0)
	{
		while(1)
		{
			fprintf(stdout, "wait for your ctrl + c exit\n");
			if(flag)
			{
				//g_main_loop_quit (ld_state->loop);
				flag = 0;
				break;
			}
			sleep(1);
		}
	}
	else{
		while(i > 0)
		{
			fprintf(stdout, "photo_shot_wait %d s\n", i);
			i = i -1;
			if(flag)
			{
				//g_main_loop_quit (ld_state->loop);
				flag = 0;
				break;
			}
			sleep(1);
		}
	}
	ld_state->fake_sink_id = g_signal_connect(G_OBJECT(ld_state->fake_sink), "handoff", G_CALLBACK(buffer_probe_callback), ld_state);
	return NULL;
}

static gboolean photo_bus_callback(GstBus *bus, GstMessage *message, gpointer data)
{
	LDCAM_STATE *state = (LDCAM_STATE *)data;
	switch(GST_MESSAGE_TYPE(message))
	{
		case GST_MESSAGE_ERROR:
			fprintf(stdout, "photo bus error message\n");
			g_main_loop_quit (state->loop);
			break;
		
		case GST_MESSAGE_WARNING:
			fprintf(stdout, "photo bus warning message\n");
			break;
		
		case GST_MESSAGE_EOS:
			fprintf(stdout, "photo bus eos message\n");
			g_main_loop_quit (state->loop);
			break;
		
		case GST_MESSAGE_STATE_CHANGED:
			//fprintf(stdout, "photo bus state changed message\n");
			break;

		default:
			break;

	}
	return TRUE;
}

int shot_thread_create(LDCAM_STATE *state)
{
	pthread_t thread_id = 0;
	pthread_attr_t  attr;
	
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	int err = pthread_create(&thread_id, NULL, photo_shot, state);
	if(err || !thread_id)
	{
		fprintf(stderr, "create thread fail\n");
		return -1;
	}
	pthread_attr_destroy(&attr);

	return 0;
}

int ldcam_photo(LDCAM_STATE *state)
{
	fprintf(stdout, "create gst work for photo\n");


	GstCaps *caps;
	GstBus *bus;
	guint bus_watch_id;

	GstStateChangeReturn ret;

	char dev_port[32] = "/dev/video";
	strcat(dev_port, state->dev);

	gst_init(NULL, NULL);
	state->loop = g_main_loop_new (NULL, FALSE);

	state->pipeline = gst_pipeline_new ("pipeline");
	//state->camera_src = gst_element_factory_make("v4l2src", "camera_src");
	state->camera_src = gst_element_factory_make("lindeniv4l2src", "camera_src");
	state->src_filter = gst_element_factory_make("capsfilter", "src_filter");

	state->video_cov = gst_element_factory_make("videoconvert", "video_cov");
	state->sunxi_sink = gst_element_factory_make("sunxifbsink", "sunxi_sink");
	
	state->jpeg_enc = gst_element_factory_make("jpegenc", "jpeg_enc");
	state->fake_sink = gst_element_factory_make("fakesink", "fake_sink");

	state->tee = gst_element_factory_make("tee", "tee");
	state->screen_queue = gst_element_factory_make("queue", "screen_queue");
	state->fake_queue = gst_element_factory_make("queue", "fake_queue");

	if(!state->pipeline || !state->camera_src || !state->sunxi_sink ||
			!state->jpeg_enc || !state->fake_sink || !state->tee ||
			!state->screen_queue || !state->fake_queue)
	{
		fprintf(stdout, "create gstreamer element fail \n");
		return -1;
	}

	g_object_set(G_OBJECT (state->camera_src), "device", dev_port, NULL);
	g_object_set(G_OBJECT(state->fake_sink), "signal-handoffs", TRUE, NULL);
	g_object_set(G_OBJECT(state->sunxi_sink), "video-memory", 32, NULL);
	//g_object_set(G_OBJECT(state->sunxi_sink), "full-screen", TRUE, NULL);
	//g_object_set(G_OBJECT(state->jpeg_enc), "quality", 100, NULL);

	bus = gst_pipeline_get_bus(GST_PIPELINE (state->pipeline));
	bus_watch_id = gst_bus_add_watch (bus, (GstBusFunc)photo_bus_callback, state->loop);
	gst_object_unref(bus);

	if(state->display)
	{
		gst_bin_add_many (GST_BIN (state->pipeline), state->camera_src, state->src_filter,
				state->video_cov, state->tee, state->screen_queue, state->sunxi_sink,
				state->fake_queue, state->jpeg_enc, state->fake_sink, NULL);
	}
	else{
		gst_bin_add_many (GST_BIN (state->pipeline), state->camera_src, state->src_filter,
				state->jpeg_enc, state->fake_sink, NULL);

	}

	if(state->display)
	{
		caps = gst_caps_new_simple ("video/x-raw",
				"format", G_TYPE_STRING, "I420",
				"width", G_TYPE_INT, state->width,
				"height", G_TYPE_INT, state->height,
				NULL);
	}
	else{
		caps = gst_caps_new_simple ("video/x-raw",
				"format", G_TYPE_STRING, "NV12",
				"width", G_TYPE_INT, state->width,
				"height", G_TYPE_INT, state->height,
				NULL);

	}
	if(!gst_element_link_filtered(state->camera_src, state->src_filter, caps))
	{
		fprintf(stderr, "link src_filter element fail \n");
		return -1;
	}
	gst_caps_unref(caps);

	if(state->display)
	{
		if(!gst_element_link_many (state->src_filter, state->video_cov, state->tee, state->screen_queue, state->sunxi_sink, NULL))
		{
			fprintf(stderr, "link screen_queue fail\n");
			return -1;
		}
		if(!gst_element_link_many (state->tee, state->fake_queue, state->jpeg_enc, state->fake_sink, NULL))
		{
			fprintf(stderr, "link fake_queue fail\n");
			return -1;
		}
	}
	else
	{
		if(!gst_element_link_many (state->src_filter, state->jpeg_enc, state->fake_sink, NULL))
		{
			fprintf(stderr, "link only fake sink fail\n");
			return -1;
		}
	}
	ret = gst_element_set_state (state->pipeline, GST_STATE_PLAYING);

	if(ret == GST_STATE_CHANGE_FAILURE)
	{
		fprintf(stderr, "pipeline set playing  fail\n");
		return -1;
	}
#if 1
	if(shot_thread_create(state) != 0)
	{
		fprintf(stderr, "shot thread create fail\n");
		gst_element_set_state (state->pipeline, GST_STATE_NULL);
		gst_object_unref (state->pipeline);
		g_source_remove (bus_watch_id);
		return -1;
	}
#endif
	g_main_loop_run (state->loop);
	
	gst_element_set_state (state->pipeline, GST_STATE_NULL);
	
	gst_object_unref (state->pipeline);
	g_source_remove (bus_watch_id);
	g_main_loop_unref (state->loop);

	return 0;
}

static gboolean video_bus_callback(GstBus *bus, GstMessage *message, gpointer data)
{
	LDCAM_STATE *state = (LDCAM_STATE *)data;
	switch(GST_MESSAGE_TYPE(message))
	{
		case GST_MESSAGE_ERROR:
			fprintf(stdout, "video bus error message\n");
			g_main_loop_quit (state->loop);
			break;
		
		case GST_MESSAGE_WARNING:
			fprintf(stdout, "video bus warning message\n");
			break;
		
		case GST_MESSAGE_EOS:
			fprintf(stdout, "video bus eos message\n");
			g_main_loop_quit (state->loop);
			break;
		
		case GST_MESSAGE_STATE_CHANGED:
			//fprintf(stdout, "photo bus state changed message\n");
			break;

		default:
			break;

	}
	return TRUE;
}

void *video_exit(void *arg)
{
	fprintf(stdout, "video_exit_callback\n");
	LDCAM_STATE *ld_state = (LDCAM_STATE *)arg;
	signal(SIGINT, exit_handler);
	int i = ld_state->time;
	if(i <= 0)
	{
		while(1)
		{
			fprintf(stdout, "wait for your ctrl + c exit\n");
			if(flag)
			{
				flag = 0;
				break;
			}
			sleep(1);
		}
	}
	else{
		while(i > 0)
		{
			fprintf(stdout, "capture video wait %d s\n", i);
			i = i -1;
			if(flag)
			{
				flag = 0;
				break;
			}
			sleep(1);
		}
	}
	g_main_loop_quit (ld_state->loop);
	return NULL;
}

int video_thread_create(LDCAM_STATE *state)
{
	pthread_t thread_id = 0;
	pthread_attr_t  attr;
	
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	int err = pthread_create(&thread_id, NULL, video_exit, state);
	if(err || !thread_id)
	{
		fprintf(stderr, "create thread fail\n");
		return -1;
	}
	pthread_attr_destroy(&attr);

	return 0;
}

int ldcam_video(LDCAM_STATE *state)
{
	fprintf(stdout, "create gst work for video\n");

	GstCaps *caps;
	GstBus *bus;
	guint bus_watch_id;

	GstStateChangeReturn ret;

	char dev_port[32] = "/dev/video";
	strcat(dev_port, state->dev);

	gst_init(NULL, NULL);
	state->loop = g_main_loop_new (NULL, FALSE);

	state->pipeline = gst_pipeline_new ("pipeline");
	//state->camera_src = gst_element_factory_make("v4l2src", "camera_src");
	state->camera_src = gst_element_factory_make("lindeniv4l2src", "camera_src");
	state->src_filter = gst_element_factory_make("capsfilter", "src_filter");
	state->video_cov = gst_element_factory_make("videoconvert", "video_cov");
	state->video_covf = gst_element_factory_make("videoconvert", "video_covf");
	state->sunxi_sink = gst_element_factory_make("sunxifbsink", "sunxi_sink");

	state->file_enc = gst_element_factory_make("omxh264videoenc", "file_enc");
	//state->file_mux = gst_element_factory_make("avimux", "file_mux");
	state->file_mux = gst_element_factory_make("mpegtsmux", "file_mux");
	state->file_sink = gst_element_factory_make("filesink", "file_sink");

	state->alsa_src = gst_element_factory_make("alsasrc", "alsa_src");
	state->alsa_filter = gst_element_factory_make("capsfilter", "alsa_filter");
	//state->alsa_enc = gst_element_factory_make("lamemp3enc", "alsa_enc");
	state->alsa_enc = gst_element_factory_make("voaacenc", "alsa_enc");

	state->tee = gst_element_factory_make("tee", "tee");
	state->screen_queue = gst_element_factory_make("queue", "screen_queue");
	state->file_queue = gst_element_factory_make("queue", "file_queue");
	state->alsa_queue = gst_element_factory_make("queue", "alsa_queue");

	if(!state->pipeline || !state->camera_src || !state->src_filter || !state->sunxi_sink ||
			!state->file_enc || !state->file_mux || !state->file_sink ||
			!state->alsa_src || !state->alsa_filter || !state->alsa_enc || 
			!state->tee || !state->screen_queue || !state->file_queue || !state->alsa_queue)
	{
		fprintf(stdout, "create gstreamer element fail \n");
		return -1;
	}

	g_object_set(G_OBJECT (state->camera_src), "device", dev_port, NULL);
	g_object_set(G_OBJECT (state->camera_src), "do-timestamp", TRUE, NULL);
	g_object_set(G_OBJECT(state->sunxi_sink), "video-memory", 32, NULL);
	//g_object_set(G_OBJECT(state->sunxi_sink), "full-screen", TRUE, NULL);

	g_object_set(G_OBJECT(state->file_sink), "location", state->path, NULL);

	g_object_set(G_OBJECT (state->alsa_src), "do-timestamp", TRUE, NULL);
	//g_object_set(G_OBJECT (state->alsa_src), "provide-clock", FALSE, NULL);
	
	bus = gst_pipeline_get_bus(GST_PIPELINE (state->pipeline));
	bus_watch_id = gst_bus_add_watch (bus, (GstBusFunc)video_bus_callback, state->loop);
	gst_object_unref(bus);

#ifdef VIDEO_AUDIO
	if(state->display)
	{
		gst_bin_add_many (GST_BIN (state->pipeline), state->camera_src, state->src_filter,
				state->video_cov, state->tee, state->screen_queue, state->sunxi_sink,
				state->file_queue, state->video_covf, state->file_enc, state->file_mux, state->file_sink,
				state->alsa_src, state->alsa_queue, state->alsa_filter, state->alsa_enc, state->file_mux, NULL);
	}
	else{
		gst_bin_add_many (GST_BIN (state->pipeline), state->camera_src, state->src_filter,
				state->file_enc, state->file_mux, state->file_sink,
				state->alsa_src, state->alsa_filter, state->alsa_enc, state->file_mux, NULL);
	}
#else
	if(state->display)
	{
		gst_bin_add_many (GST_BIN (state->pipeline), state->camera_src, state->src_filter,
				state->video_cov, state->tee, state->screen_queue, state->sunxi_sink,
				state->file_queue, state->video_covf, state->file_enc, state->file_mux, state->file_sink, NULL);
	}
	else{
		gst_bin_add_many (GST_BIN (state->pipeline), state->camera_src, state->src_filter,
				state->file_enc, state->file_mux, state->file_sink, NULL);
	}
#endif
	if(state->display)
	{
		caps = gst_caps_new_simple ("video/x-raw",
				"width", G_TYPE_INT, state->width,
				"height", G_TYPE_INT, state->height,
				"format", G_TYPE_STRING, "I420",
				NULL);
	}
	else{
		caps = gst_caps_new_simple ("video/x-raw",
				"width", G_TYPE_INT, state->width,
				"height", G_TYPE_INT, state->height,
				"format", G_TYPE_STRING, "NV12",
				NULL);
	}
	if(!gst_element_link_filtered(state->camera_src, state->src_filter, caps))
	{
		fprintf(stderr, "link src_filter element fail \n");
		return -1;
	}
	gst_caps_unref(caps);
#ifdef VIDEO_AUDIO 
	caps = gst_caps_new_simple ("audio/x-raw",
			"format", G_TYPE_STRING, "S16LE",
			"rate", G_TYPE_INT, (int)44100,
			"channels", G_TYPE_INT, (int)2, NULL);
	if(!gst_element_link_filtered(state->alsa_src, state->alsa_filter, caps))
	{
		fprintf(stderr, "link alsa_filter element fail \n");
		return -1;
	}
	gst_caps_unref(caps);

#endif
	if(state->display)
	{
		if(!gst_element_link_many (state->src_filter, state->video_cov, state->tee, state->screen_queue, state->sunxi_sink, NULL))
		{
			fprintf(stderr, "link screen_queue fail\n");
			return -1;
		}
		if(!gst_element_link_many (state->tee, state->file_queue, state->video_covf, state->file_enc, state->file_mux, state->file_sink, NULL))
		{
			fprintf(stderr, "link file_queue fail\n");
			return -1;
		}
#ifdef VIDEO_AUDIO 
		if(!gst_element_link_many (state->alsa_filter, state->alsa_queue, state->alsa_enc, state->file_mux, NULL))
		{
			fprintf(stderr, "link alsa_queue fail\n");
			return -1;
		}
#endif
	}
	else{
		if(!gst_element_link_many (state->src_filter, state->file_enc, state->file_mux, state->file_sink, NULL))
		{
			fprintf(stderr, "link file_queue fail\n");
			return -1;
		}
#ifdef VIDEO_AUDIO 
		if(!gst_element_link_many (state->alsa_filter, state->alsa_enc, state->file_mux, NULL))
		{
			fprintf(stderr, "link alsa_queue fail\n");
			return -1;
		}
#endif
	}

	ret = gst_element_set_state (state->pipeline, GST_STATE_PLAYING);
	
	if(ret == GST_STATE_CHANGE_FAILURE)
	{
		fprintf(stderr, "pipeline set playing  fail\n");
		return -1;
	}
#if 1
	if(video_thread_create(state) != 0)
	{
		fprintf(stderr, "shot thread create fail\n");
		gst_element_set_state (state->pipeline, GST_STATE_NULL);
		gst_object_unref (state->pipeline);
		g_source_remove (bus_watch_id);
		return -1;
	}
#endif
	g_main_loop_run (state->loop);
	
	gst_element_set_state (state->pipeline, GST_STATE_NULL);
	
	gst_object_unref (state->pipeline);
	g_source_remove (bus_watch_id);
	g_main_loop_unref (state->loop);

	return 0;
}

int main(int argc, const char **argv)
{
	LDCAM_STATE *state;
	
	int ret = 0;

	state = (LDCAM_STATE *)malloc(sizeof(LDCAM_STATE));

	default_state(state);

	if(argc == 1 || argv[1][1] == '?' || !strcmp("--help", argv[1]))
	{
		display_cmd_parameters(argv[0]);
		return -1;
	}

	if(paser_cmdline(argc, argv, state) == -1)
		return -1;

	if(state->mode == 0)
	{
		ret = ldcam_photo(state);
		if(ret != 0)
			return ret;
	}
	else if(state->mode == 1)
	{
		ret = ldcam_video(state);
		if(ret != 0)
			return ret;
	}

	free(state);
	
	return 0;
}
