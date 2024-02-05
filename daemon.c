#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <sys/inotify.h>

#include <libnotify/notify.h>

#define EXIT_SUCCESS 0
#define EXIT_ERR_TOO_FEW_ARGS 1
#define EXIT_ERR_INIT_INOTIFY 2
#define EXIT_ERR_ADD_WATCH 3
#define EXIT_ERR_BASE_PATH_NULL 4
#define EXIT_ERR_READ_INOTIFY 5
#define EXIT_ERR_INIT_LIBNOTIFY 6

int IEventQueue = -1;
int IEventStatus = -1;

char *ProgramTitle = "DAEMON";

void signal_handler(int signal) {
    
    int closed = -1;
    printf("Signal received, cleaning up... \n");

    closed = inotify_rm_watch(IEventQueue, IEventStatus);
    if (closed == -1) {
        fprintf(stderr, "Error from watch queue! \n");
    }

    close(IEventQueue);
    exit(EXIT_SUCCESS);
}

int main(int argc, char** argv) {

    bool libnotifyinitstatus;

    const struct inotify_event* watch_event;

    char *basePath = NULL;
    char *token = NULL;
    char *notification_msg = NULL;

    NotifyNotification *notifyHandle;

    char buffer[4096];
    int readLength;

    const uint32_t watch_mask = IN_CREATE | IN_DELETE | IN_ACCESS | IN_CLOSE | IN_MODIFY | IN_MOVE;
    
    if (argc < 2) {
        fprintf(stderr, "USAGE: daemon PATH \n");
        exit(EXIT_ERR_TOO_FEW_ARGS);
    }

    basePath = (char *)malloc(sizeof(char)*(strlen(argv[1]) + 1));
    strcpy(basePath, argv[1]);

    token = strtok(basePath, "/");
    while (token != NULL) {
        basePath = token;
        token = strtok(NULL, "/"); /* this expression means that it should continue tokenizing the same string which is `basePath`*/
    }

    if (basePath == NULL) {
        fprintf(stderr, "Error getting base path");
        exit(EXIT_ERR_BASE_PATH_NULL);
    }

    libnotifyinitstatus = notify_init(ProgramTitle);
    if(!libnotifyinitstatus) {
        fprintf(stderr, "Error initialising libnotify \n");
        exit(EXIT_ERR_INIT_LIBNOTIFY);
    }

    IEventQueue = inotify_init();
    if (IEventQueue == -1) {
        fprintf(stderr, "Error initialising inotify instance \n");
        exit(EXIT_ERR_INIT_INOTIFY);
    }

    IEventStatus = inotify_add_watch(IEventQueue, argv[1], watch_mask);
    if (IEventStatus == -1) {
        fprintf(stderr, "Error adding file to watch instance! \n");
        exit(EXIT_ERR_ADD_WATCH);
    }    

    signal(SIGABRT, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    while (true) {
        printf("waiting for ievent... \n");

        readLength = read(IEventQueue, buffer, sizeof(buffer));
        if (readLength == -1) {
            fprintf(stderr, "Error reading from inotify instance");
            exit(EXIT_ERR_READ_INOTIFY);
        }

        for (char *bufPointer = buffer; bufPointer < buffer + readLength; bufPointer += sizeof(struct inotify_event)+watch_event->len) {
            notification_msg = NULL;
            
            watch_event = (const struct inotify_event *) bufPointer;

            if (watch_event->mask & IN_CREATE ) {
                notification_msg = "file created";
            }

            if (watch_event->mask & IN_DELETE) {
                notification_msg = "file deleted";
            }

            if (watch_event->mask & IN_ACCESS) {
                notification_msg = "file accessed";
            }

            if (watch_event->mask & IN_CLOSE) {
                notification_msg = "file closed";
            }

            if (watch_event->mask & IN_MODIFY) {
                notification_msg = "file modified";
            }

            if (watch_event->mask & IN_MOVE) {
                notification_msg = "file moved";
            }

            if (notification_msg == NULL) {
                continue;
            }

            notifyHandle = notify_notification_new(basePath, notification_msg, "dialog-information");
            if(notifyHandle == NULL) {
                fprintf(stderr, "notification handle was null \n");
                continue;
            }

            notify_notification_show(notifyHandle, NULL);
        }
    }
}