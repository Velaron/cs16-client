package in.celest.xash3d.cs16client;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.content.SharedPreferences;
import java.io.FileOutputStream;
import java.io.File;
import java.io.InputStream;
import java.lang.reflect.Method;

public class InstallReceiver extends BroadcastReceiver 
{
	private static final String TAG = "CS16CLIENT_LAUNCHER";
	static SharedPreferences mPref;
	@Override
	public void onReceive( Context context, Intent arg1 ) 
	{
		Log.d( TAG, "Install received, extracting PAK" );
		extractPAK( context, true );
    }
    
    private static int chmod(String path, int mode) {
		int ret = -1;
		try
		{
			ret = Runtime.getRuntime().exec("chmod " + Integer.toOctalString(mode) + " " + path).waitFor();
			Log.d(TAG, "chmod " + Integer.toOctalString(mode) + " " + path + ": " + ret );
		}
		catch(Exception e)
		{
			ret = -1;
			Log.d(TAG, "chmod: Runtime not worked: " + e.toString() );
		}
		try
		{
			Class fileUtils = Class.forName("android.os.FileUtils");
			Method setPermissions = fileUtils.getMethod("setPermissions",
				String.class, int.class, int.class, int.class);
			ret = (Integer) setPermissions.invoke(null, path,
				mode, -1, -1);
		}
		catch(Exception e)
		{
			ret = -1;
			Log.d(TAG, "chmod: FileUtils not worked: " + e.toString() );
		}
		return ret;
	 }

	 private static void extractFile(Context context, String path) 
	 {
		try
		{
			InputStream is = null;
			FileOutputStream os = null;
			is = context.getAssets().open(path);
			File out = new File(context.getFilesDir().getPath()+'/'+path);
			out.getParentFile().mkdirs();
			chmod( out.getParent(), 0777 );
			os = new FileOutputStream(out);
			byte[] buffer = new byte[1024];
			int length;
			while ((length = is.read(buffer)) > 0) 
			{
				os.write(buffer, 0, length);
			}
			os.close();
			is.close();
			chmod( context.getFilesDir().getPath()+'/'+path, 0777 );
		} 
		catch( Exception e )
		{
			Log.e( TAG, "Failed to extract file:" + e.toString() );
			e.printStackTrace();
		}
	 }

	private static synchronized void extractPAK(Context context, Boolean force) 
	{
		InputStream is = null;
		FileOutputStream os = null;
		try 
		{
			if( mPref == null )
				mPref = context.getSharedPreferences("mod", 0);
			synchronized( mPref )
			{
				if( mPref.getInt( "pakversion", 0 ) == LauncherActivity.PAK_VERSION && !force )
					return;
				extractFile(context, "extras.pak");
				SharedPreferences.Editor editor = mPref.edit();
				editor.putInt( "pakversion", LauncherActivity.PAK_VERSION );
				editor.commit();
			}
		} 
		catch( Exception e )
		{
			Log.e( TAG, "Failed to extract PAK:" + e.toString() );
		}
	}
}
