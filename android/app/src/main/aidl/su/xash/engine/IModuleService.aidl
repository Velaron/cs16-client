package su.xash.engine;

interface IModuleService {
    int getVersion();
    boolean checkIfSupported(String title, String gamedir);
    String getArguments(String args, String gamedir);
    List<String> getEnvironment();
    String getLibraryPath();
}
