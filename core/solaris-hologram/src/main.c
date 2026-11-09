#include "common.h"
#include "layer_window.h"
#include <signal.h>

static hologram_app_t g_app;

static void sig_handler(int sig) {
    if (sig == SIGUSR1) {
        // Toggle Command Center mode
        layer_window_toggle_mode(&g_app);
    } else if (sig == SIGTERM || sig == SIGINT) {
        if (gtk_main_level() > 0) {
            gtk_main_quit();
        } else {
            exit(0);
        }
    }
}

static void print_usage(const char *prog) {
    printf("🛸 SOLARIS 3D HOLOGRAPHIC WORKSPACE VISUALIZATION & CONTROLLER\n");
    printf("Usage:\n");
    printf("  %s [options]\n\n", prog);
    printf("Options:\n");
    printf("  --background, -b      Run as desktop background layer (default)\n");
    printf("  --orbital             Switch central widget to ORBITAL planet mode\n");
    printf("  --server              Switch central widget to 3D SERVER CORE mode\n");
    printf("  --terminal            Switch central widget to HOLOGRAPHIC TERMINAL mode\n");
    printf("  --toggle-core         Toggle between ORBITAL and SERVER CORE\n");
    printf("  --command-center, -c  Launch directly into fullscreen Command Center HUD\n");
    printf("  --toggle, -t          Toggle Command Center HUD overlay on running instance\n");
    printf("  --status, -s          Print current Niri workspaces and windows status\n");
    printf("  --kill, -k            Terminate running hologram processes\n");
    printf("  --help, -h            Show this help manual\n");
}

int main(int argc, char **argv) {
    gboolean start_command_center = FALSE;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--orbital") == 0) {
            FILE *fp = fopen("/tmp/solaris_core_mode", "w");
            if (fp) { fputs("orbital\n", fp); fclose(fp); }
            printf("[Solaris Core] Switched to ORBITAL mode.\n");
            return 0;
        } else if (strcmp(argv[i], "--server") == 0) {
            FILE *fp = fopen("/tmp/solaris_core_mode", "w");
            if (fp) { fputs("server\n", fp); fclose(fp); }
            printf("[Solaris Core] Switched to 3D SERVER CORE mode.\n");
            return 0;
        } else if (strcmp(argv[i], "--terminal") == 0) {
            FILE *fp = fopen("/tmp/solaris_core_mode", "w");
            if (fp) { fputs("terminal\n", fp); fclose(fp); }
            printf("[Solaris Core] Switched to HOLOGRAPHIC TERMINAL mode.\n");
            return 0;
        } else if (strcmp(argv[i], "--toggle-core") == 0) {
            FILE *fp = fopen("/tmp/solaris_core_mode", "w");
            if (fp) { fputs("toggle\n", fp); fclose(fp); }
            printf("[Solaris Core] Toggled central core mode.\n");
            return 0;
        } else if (strcmp(argv[i], "--command-center") == 0 || strcmp(argv[i], "-c") == 0) {
            start_command_center = TRUE;
        } else if (strcmp(argv[i], "--toggle") == 0 || strcmp(argv[i], "-t") == 0) {
            // Signal running instance or launch
            FILE *fp = popen("pgrep -f 'solaris-hologram|solaris-bg-hologram'", "r");
            if (fp) {
                char pid_buf[64];
                gboolean signaled = FALSE;
                pid_t my_pid = getpid();
                while (fgets(pid_buf, sizeof(pid_buf), fp)) {
                    pid_t pid = atoi(pid_buf);
                    if (pid > 0 && pid != my_pid) {
                        kill(pid, SIGUSR1);
                        signaled = TRUE;
                    }
                }
                pclose(fp);
                if (signaled) {
                    printf("[Solaris] Toggled Command Center overlay.\n");
                    return 0;
                }
            }
            // If none running, launch command center directly
            start_command_center = TRUE;
        } else if (strcmp(argv[i], "--kill") == 0 || strcmp(argv[i], "-k") == 0 || strcmp(argv[i], "--quit") == 0) {
            system("pkill -f 'solaris-hologram|solaris-bg-hologram' 2>/dev/null || true");
            printf("[Solaris] Hologram processes stopped.\n");
            return 0;
        } else if (strcmp(argv[i], "--status") == 0 || strcmp(argv[i], "-s") == 0) {
            system("niri msg --json workspaces 2>/dev/null || echo 'Niri IPC offline'");
            return 0;
        }
    }

    gtk_init(&argc, &argv);

    // Setup signal handling
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sig_handler;
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    memset(&g_app, 0, sizeof(g_app));
    if (!layer_window_init(&g_app, start_command_center)) {
        fprintf(stderr, "[Solaris Error] Failed to initialize Holographic Layer Window.\n");
        return 1;
    }

    printf("[Solaris] 🛸 3D Holographic Workspace Controller online.\n");
    gtk_main();

    niri_ipc_cleanup(&g_app.ipc);
    orbital_system_cleanup(&g_app.sys);

    return 0;
}
