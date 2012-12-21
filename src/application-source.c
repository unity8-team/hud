#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "application-source.h"

struct _HudApplicationSourcePrivate {
	int dummy;
};

#define HUD_APPLICATION_SOURCE_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), HUD_APPLICATION_SOURCE_TYPE, HudApplicationSourcePrivate))

static void hud_application_source_class_init (HudApplicationSourceClass *klass);
static void hud_application_source_init       (HudApplicationSource *self);
static void hud_application_source_dispose    (GObject *object);
static void hud_application_source_finalize   (GObject *object);

G_DEFINE_TYPE (HudApplicationSource, hud_application_source, G_TYPE_OBJECT);

/* Class Init */
static void
hud_application_source_class_init (HudApplicationSourceClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (HudApplicationSourcePrivate));

	object_class->dispose = hud_application_source_dispose;
	object_class->finalize = hud_application_source_finalize;

	return;
}

/* Instance Init */
static void
hud_application_source_init (HudApplicationSource *self)
{

	return;
}

/* Clean up references */
static void
hud_application_source_dispose (GObject *object)
{

	G_OBJECT_CLASS (hud_application_source_parent_class)->dispose (object);
	return;
}

/* Free memory */
static void
hud_application_source_finalize (GObject *object)
{

	G_OBJECT_CLASS (hud_application_source_parent_class)->finalize (object);
	return;
}

HudApplicationSource *
hud_application_source_new_for_app (BamfApplication * bapp)
{

	return NULL;
}

gboolean
hud_application_source_is_empty (HudApplicationSource * app)
{

	return TRUE;
}

gchar *
hud_application_source_bamf_app_id (BamfApplication * bapp)
{

	return g_strdup("");
}
