package su.xash.cs16client;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

import su.xash.engine.IModuleService;

public class ModuleService extends Service {
    private static final int INTERFACE_VERSION = 1;

    private final IModuleService.Stub binder = new IModuleService.Stub() {
        @Override
        public int getVersion() {
            return INTERFACE_VERSION;
        }

        @Override
        public boolean checkIfSupported(String title, String gamedir) {
            List<String> supportedGamedirs = Arrays.asList(
                    "cstrike",
                    "czero"
            );
            return supportedGamedirs.contains(gamedir);
        }

        @Override
        public String getArguments(String args, String gamedir) {
            List<String> tokens = new ArrayList<>(Arrays.asList(args.split(" ")));

            int index = tokens.indexOf("-dll");
            if (index == -1) {
                tokens.add("-dll");
                tokens.add("@yapb");
            }

            return String.join(" ", tokens);
        }

        @Override
        public List<String> getEnvironment() {
            return Collections.emptyList();
        }

        @Override
        public String getLibraryPath() {
            return getApplicationInfo().nativeLibraryDir;
        }
    };

    @Override
    public IBinder onBind(Intent intent) {
        return binder;
    }
}
