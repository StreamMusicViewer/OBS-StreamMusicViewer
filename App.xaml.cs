using System.Threading;
using System.Windows;

namespace OBS_StreamMusicViewer
{
    public partial class App : Application
    {
        private static Mutex _mutex;

        protected override void OnStartup(StartupEventArgs e)
        {
            const string mutexName = "OBS-StreamMusicViewer-SingleInstance";
            _mutex = new Mutex(true, mutexName, out bool isNewInstance);

            if (!isNewInstance)
            {
                MessageBox.Show(
                    "OBS Stream Music Viewer est déjà en cours d'exécution.\n\nVérifiez l'icône dans la barre des tâches (zone de notification).",
                    "Application déjà lancée",
                    MessageBoxButton.OK,
                    MessageBoxImage.Information);

                _mutex = null;
                Shutdown();
                return;
            }

            base.OnStartup(e);
        }

        protected override void OnExit(ExitEventArgs e)
        {
            _mutex?.ReleaseMutex();
            _mutex?.Dispose();
            base.OnExit(e);
        }
    }
}
