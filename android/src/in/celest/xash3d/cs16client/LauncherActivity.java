/*
*
*    This program is free software; you can redistribute it and/or modify it
*    under the terms of the GNU General Public License as published by the
*    Free Software Foundation; either version 2 of the License, or (at
*    your option) any later version.
*
*    This program is distributed in the hope that it will be useful, but
*    WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*    In addition, as a special exception, the author gives permission to
*    link the code of this program with the Half-Life Game Engine ("HL
*    Engine") and Modified Game Libraries ("MODs") developed by Valve,
*    L.L.C ("Valve").  You must obey the GNU General Public License in all
*    respects for all of the code used other than the HL Engine and MODs
*    from Valve.  If you modify this file, you may extend this exception
*    to your version of the file, but you are not obligated to do so.  If
*    you do not wish to do so, delete this exception statement from your
*    version.
*
*/
package in.celest.xash3d.cs16client;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.content.ComponentName;
import android.content.pm.PackageManager;
import android.content.SharedPreferences;
import android.content.DialogInterface;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.Window;
import android.widget.EditText;
import android.widget.CheckBox;
import android.widget.ToggleButton;
import android.widget.TextView;
import android.widget.TabHost;
import android.os.Environment;
import android.os.Build;
import android.net.Uri;
import android.util.DisplayMetrics;
import java.io.FileOutputStream;
import java.io.File;
import java.io.InputStream;
import java.lang.reflect.Method;

import com.google.android.gms.ads.*;
import com.google.android.gms.common.GoogleApiAvailability;
import com.google.android.gms.common.ConnectionResult;

import in.celest.xash3d.cs16client.R;

public class LauncherActivity extends Activity {
	public static final int PAK_VERSION = 4;
	public final static int sdk = Integer.valueOf(Build.VERSION.SDK);
	public final static String TAG = "LauncherActivity";
	
	public static Context mContext;
	static SharedPreferences mPref;
	static EditText mCmdArgs;
	static EditText mBaseDir;
	static ToggleButton mEnableZBot;
	static ToggleButton mEnableYaPB;
	static ToggleButton mEnableCZero;
	static AdView   mAdView;
	static TextView mTitle;
	static Boolean isExtracting = false;
	static int mClicks;
	static Boolean mDev;
	static Boolean mFirstTime;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		this.requestWindowFeature(Window.FEATURE_NO_TITLE);
		
		// disable as there is not stupid holo blue tabs
		//if ( sdk >= 21 )
		//	super.setTheme( 0x01030224 );
		//else super.setTheme( 0x01030005 );

		setContentView(R.layout.activity_launcher);
		
		mContext = getApplicationContext();
		
		// get preferences
		mPref          = getSharedPreferences("mod", 0);
		
		mCmdArgs       = (EditText)findViewById(R.id.cmdArgs);
		mEnableZBot    = (ToggleButton)findViewById(R.id.use_zbot);
		mEnableYaPB    = (ToggleButton)findViewById(R.id.use_yapb);
		mEnableCZero   = (ToggleButton)findViewById(R.id.enableczero); // TODO
		mAdView        = (AdView)  findViewById(R.id.adView);
		mTitle         = (TextView) findViewById(R.id.textView_tittle);
		mClicks = 0;
		mDev = mPref.getBoolean("dev", false);
		
		// TODO: extend firsttime with requesting player nickname!
		mFirstTime = mPref.getBoolean( "firsttime", true );
		
		mCmdArgs.   setText   (mPref.getString ("argv"   , "-console"));
		mEnableZBot.setChecked(mPref.getBoolean("zbots"  , true ));
		mEnableYaPB.setChecked(mPref.getBoolean("yapbs"  , false));
		mEnableCZero.setChecked(mPref.getBoolean("czero" , false)); // TODO
		
		if( !mDev )
			mEnableCZero.setVisibility(View.GONE);
		
		try
		{
			AdRequest adRequest = new AdRequest.Builder()
				.addTestDevice(AdRequest.DEVICE_ID_EMULATOR)
				.build();
			mAdView.loadAd(adRequest);
		}
		catch(Throwable e)
		{
			// Very dirty hack!
			// AdMob uses some methods that are not available on some devices
			// Just don't let app crash.
			Log.e( TAG, "Something happened during load ad. " + e.getMessage() );
		}
		
		if( mFirstTime )
		{
			showTutorial();
		}
	}

	public void startXash(View view)
	{
		SharedPreferences.Editor editor = mPref.edit();
		String argv    = mCmdArgs.getText().toString();
		String gamedir = "cstrike";

		editor.putString("argv", argv);
		editor.putBoolean("zbots", mEnableZBot.isChecked());
		editor.putBoolean("yapbs", mEnableYaPB.isChecked());
		if( mFirstTime ) editor.putBoolean("firsttime", false);
		editor.commit();

		extractPAK(this, false);

		if( mEnableYaPB.isChecked() )
		{
			String fullPath = getFilesDir().getAbsolutePath().replace("/files","/lib");
			File yapb_hardfp = new File( fullPath + "/libyapb_hardfp.so" );
			File yapb = new File( fullPath + "/libyapb.so" );
			if( yapb_hardfp.exists() && !yapb_hardfp.isDirectory() )
				argv = argv + " -dll " + yapb_hardfp.getAbsolutePath();
			else if( yapb.exists() && !yapb.isDirectory() )
				argv = argv + " -dll " + yapb.getAbsolutePath();
			else
			{
				Log.v(TAG, "YaPB not found!");
				AlertDialog.Builder notFoundDialogBuilder = new AlertDialog.Builder(this);
				notFoundDialogBuilder.setMessage(R.string.not_found_msg)
					.setTitle(R.string.not_found_title);
				notFoundDialogBuilder.create().show();
				return;
			}
		}
		
		if( mEnableZBot.isChecked() )
		{
			argv = argv + " -bots";
		}
		
		if( mEnableCZero.isChecked() ) // TODO
		{
			AlertDialog.Builder notImplementedDialogBuilder = new AlertDialog.Builder(this);
			notImplementedDialogBuilder.setMessage(R.string.not_implemented_msg)
				.setTitle(R.string.not_implemented_fully);
			notImplementedDialogBuilder.create().show();
			
			gamedir = "czero"; // use when czero will be done
		}
		
		// TODO:
		// Check is installed engine is Google Play version
		// Check myself for GP version
		argv = argv + " -noch"; 
		
		if( mFirstTime )
		{
			argv = argv + " -firsttime umu"; // pass argument, because xash have a bug related to client's CheckParm
		}
		
		Intent intent = new Intent();
		intent.setAction("in.celest.xash3d.START");
		intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

		intent.putExtra("argv",       argv);
		intent.putExtra("gamedir",    gamedir );
		intent.putExtra("gamelibdir", getFilesDir().getAbsolutePath().replace("/files","/lib"));
		intent.putExtra("pakfile",    getFilesDir().getAbsolutePath() + "/extras.pak" );
		
		PackageManager pm = getPackageManager();
		if( intent.resolveActivity( pm ) != null )
		{
			startActivity( intent );
		}
		else
		{
			showXashInstallDialog( );
		}
	}
	
	public void showTutorial( )
	{
		AlertDialog.Builder builder = new AlertDialog.Builder( this );
		
		// TODO: must be less dumb someday...
		builder.setTitle( R.string.first_run_reminder_title )
			.setMessage( R.string.first_run_reminder_msg )
			.setNeutralButton( R.string.ok, new DialogInterface.OnClickListener() { public void onClick( DialogInterface dialog, int which ) { } } )
			.show();
	}

	
	public void showXashInstallDialog( )
	{
		AlertDialog.Builder builder = new AlertDialog.Builder( this );
		
		builder.setTitle( R.string.xash_not_installed_title )
			.setMessage( R.string.xash_not_installed_msg )
			.setPositiveButton( R.string.install_xash, 
			new DialogInterface.OnClickListener()
			{
				@Override
				public void onClick( DialogInterface dialog, int which )
				{
					GoogleApiAvailability api = GoogleApiAvailability.getInstance();
					int avail = api.isGooglePlayServicesAvailable( LauncherActivity.mContext );
					
					if( avail == ConnectionResult.SUCCESS )
					{
						// open GP
						try 
						{
							startActivity( 
								new Intent( Intent.ACTION_VIEW, 
									Uri.parse("market://details?id=in.celest.xash3d.hl") ) );
						} 
						catch( android.content.ActivityNotFoundException e ) 
						{
							startActivity(
								new Intent( Intent.ACTION_VIEW, 
									Uri.parse("https://play.google.com/store/apps/details?id=in.celest.xash3d.hl" ) ) );
						}
					}
					else
					{
						try
						{
							startActivity( 
								new Intent( Intent.ACTION_VIEW, 
									Uri.parse("https://github.com/FWGS/xash3d/releases/latest") ) );
						}
						catch( Exception e )
						{ }
					}
				}
			} )
			.setNegativeButton( R.string.cancel, 
			new DialogInterface.OnClickListener()
			{
				@Override
				public void onClick( DialogInterface dialog, int which )
				{
					dialog.cancel();
				}
			} )
			.show();
	}
	
	public void onTitleClick(View view)
	{
		if( mDev )
			return;
		
		if( mClicks++ > 10 )
		{
			SharedPreferences.Editor editor = mPref.edit();
			editor.putBoolean("dev", true);
			editor.commit();
			
			mDev = true;
			
			mEnableCZero.setVisibility(View.VISIBLE);
		}
	}

	@Override
	public void onResume() {
		super.onResume();
		if(mAdView != null)
			mAdView.resume();
	}

	@Override
	public void onDestroy() {
		if(mAdView != null)
			mAdView.destroy();

		super.onDestroy();
	}

	@Override
	public void onPause() {
		if(mAdView != null)
			mAdView.pause();
	
		super.onPause();
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

	public static void extractPAK(Context context, Boolean force) 
	{
		InputStream is = null;
		FileOutputStream os = null;
		try 
		{
			if( mPref == null )
				mPref = context.getSharedPreferences("mod", 0);
			
			if( mPref.getInt( "pakversion", 0 ) == PAK_VERSION && !force )
					return;
			extractFile(context, "extras.pak");
			SharedPreferences.Editor editor = mPref.edit();
			editor.putInt( "pakversion", PAK_VERSION );
			editor.commit();
		} 
		catch( Exception e )
		{
			Log.e( TAG, "Failed to extract PAK:" + e.toString() );
		}
	}
}

